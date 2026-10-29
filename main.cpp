#include <iostream>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <cmath>
#include <chrono>
#include <sstream>
#include <string>
#include <limits>

#include "instancia.h"
#include "resolvedor.h"
#include "tipo_prova.h"

using namespace std;

const int MAX_TIPOS_ESCOLA = 2;
const int N_EXECUCOES = 10;

struct ResultadoInstancia {
    string nome;
    bool arquivoCarregado = false;
    int numViaveis = 0;

    double melhorDist = 0.0;
    double mediaDist = 0.0;
    double piorDist = 0.0;
    double desvio = 0.0;
    double amplitude = 0.0;

    double distInicialMelhor = 0.0;
    bool inicialMelhorViavel = false;
    double melhoriaMelhor = 0.0;
    int melhorSeed = -1;

    double limiteTeorico = 0.0;
    double gapRelativo = 0.0;
    bool gapDisponivel = false;

    double tempoMedio = 0.0;
    double tempoTotal = 0.0;
};

struct DadosExecucao {
    int seed = 0;
    bool inicialViavel = false;
    bool finalViavel = false;
    double distInicial = 0.0;
    double distFinal = 0.0;
    double tempo = 0.0;
};

string formatarNumero(double valor, int casas = 2) {
    ostringstream os;
    os << fixed << setprecision(casas) << valor;
    return os.str();
}

string formatarMedida(bool disponivel, double valor, int casas = 2) {
    if (!disponivel) return "Sem solucao";
    return formatarNumero(valor, casas);
}

// Preenche os objetos LocalProva já criados pelo construtor de resolvedor.
void configurarLocais(resolvedor* r, Instancia* inst) {
    for (int j = 0; j < inst->numLocais; j++) {
        LocalProva& lp = r->locaisProva[j];
        const DadosEscola& d = inst->escolas[j];

        lp.id = j;
        lp.maxTipos = MAX_TIPOS_ESCOLA;
        lp.numSalas = d.numSalas;
        lp.lat = d.lat;
        lp.lon = d.lon;

        lp.capSalas = new int[d.numSalas];
        lp.ocupSalas = new int[d.numSalas];
        lp.tipoSalas = new int[d.numSalas];

        lp.capTotal = 0;
        for (int s = 0; s < d.numSalas; s++) {
            lp.capSalas[s] = d.caps[s];
            lp.ocupSalas[s] = 0;
            lp.tipoSalas[s] = TIPO_NENHUM;
            lp.capTotal += d.caps[s];
        }

        lp.tipos = new int[lp.maxTipos];
        for (int k = 0; k < lp.maxTipos; k++)
            lp.tipos[k] = TIPO_NENHUM;

        lp.alunosPorTipo = new int[NUM_TIPOS];
        for (int k = 0; k < NUM_TIPOS; k++)
            lp.alunosPorTipo[k] = 0;

        lp.numTipos = 0;
        lp.tiposAlocados = 0;
        lp.ocupTotal = 0;

        // Mantém a distribuição aleatória limitada às salas desta escola.
        lp.dist = uniform_int_distribution<int>(0, d.numSalas - 1);
        lp.gen.seed(42 + j);
    }
}

// O construtor de resolvedor já aloca o vetor de candidatos.
void configurarCandidatos(resolvedor* r, Instancia* inst) {
    for (int i = 0; i < inst->numCandidatos; i++) {
        r->candidatos[i].tipo = inst->tipos[i];
        r->candidatos[i].escola = ESCOLA_NENHUMA;
        r->candidatos[i].sala = SALA_NENHUMA;
    }
}

resolvedor* rodarExecucao(Instancia* inst, int seed,
                          double& distInicial,
                          bool& inicialViavel,
                          bool& finalViavel) {
    resolvedor* r = new resolvedor(inst->matDist, inst->numCandidatos,
                                   inst->numLocais, MAX_TIPOS_ESCOLA);

    configurarLocais(r, inst);
    configurarCandidatos(r, inst);

    r->seed = seed;
    r->construir();

    distInicial = r->avaliar();
    // Validação silenciosa: uma distância parcial não será tratada como solucao.
    inicialViavel = r->verificarSolucao(false);

    // buscaLocal retorna se a solucao final inteira é válida.
    finalViavel = r->buscaLocal(100);
    return r;
}

double calcularLimiteTeorico(resolvedor* r) {
    double limite = 0.0;
    for (int i = 0; i < r->numCandidatos; i++) {
        double melhor = numeric_limits<double>::infinity();
        for (int j = 0; j < r->numLocais; j++) {
            if (r->locaisProva[j].temTipo(r->candidatos[i].tipo) &&
                r->matDist[i][j] < melhor) {
                melhor = r->matDist[i][j];
            }
        }
        if (!isfinite(melhor))
            return numeric_limits<double>::quiet_NaN();
        limite += melhor;
    }
    return limite;
}

void imprimirMetricasInstancia(const ResultadoInstancia& r) {
    cout << "\n=== ESTATISTICAS (" << N_EXECUCOES << " execucoes) ===\n";
    cout << "Execucoes com solucao valida: " << r.numViaveis
         << "/" << N_EXECUCOES << "\n";

    bool encontrou = r.numViaveis > 0;
    cout << "Melhor final: " << formatarMedida(encontrou, r.melhorDist)
         << (encontrou ? " km" : "") << "\n";
    cout << "Media final:  " << formatarMedida(encontrou, r.mediaDist)
         << (encontrou ? " km" : "") << "\n";
    cout << "Pior final:   " << formatarMedida(encontrou, r.piorDist)
         << (encontrou ? " km" : "") << "\n";
    cout << "Desvio:       " << formatarMedida(encontrou, r.desvio)
         << (encontrou ? " km" : "") << "\n";
    cout << "Amplitude:    " << formatarMedida(encontrou, r.amplitude)
         << (encontrou ? " km" : "") << "\n";

    bool inicialDisponivel = encontrou && r.inicialMelhorViavel;
    cout << "Dist. inicial da melhor: "
         << formatarMedida(inicialDisponivel, r.distInicialMelhor)
         << (inicialDisponivel ? " km" : "") << "\n";
    cout << "Melhoria da melhor:      "
         << formatarMedida(inicialDisponivel, r.melhoriaMelhor)
         << (inicialDisponivel ? " km" : "") << "\n";
    cout << "Seed da melhor: "
         << (encontrou ? to_string(r.melhorSeed) : "Sem solucao") << "\n";

    cout << "Tempo medio: " << formatarNumero(r.tempoMedio) << " s\n";
    cout << "Tempo total: " << formatarNumero(r.tempoTotal) << " s\n";

    if (!encontrou) {
        cout << "Nenhuma das 10 execucoes encontrou uma solucao valida.\n";
        cout << "Isso nao prova inviabilidade matematica; indica que o algoritmo nao encontrou solucao viavel.\n";
    }
}

void imprimirDetalhesMelhor(Instancia* inst, const ResultadoInstancia& resultado) {
    if (resultado.numViaveis == 0) {
        cout << "\n=== MELHOR SOLUCAO ===\nSem solucao; detalhes e gap nao serao exibidos.\n";
        return;
    }

    cout << "\n=== REPRODUZINDO A MELHOR EXECUCAO VIAVEL (seed="
         << resultado.melhorSeed << ") ===\n";

    double distInicial = 0.0;
    bool inicialViavel = false;
    bool finalViavel = false;
    resolvedor* r = rodarExecucao(inst, resultado.melhorSeed,
                                  distInicial, inicialViavel, finalViavel);


    if (!finalViavel) {
        cout << "Distancia final: Sem solucao\n";
        cout << "A reproducao nao gerou uma solucao valida; resumo e gap omitidos.\n";
        delete r;
        return;
    }

    double atual = r->avaliar();
    r->imprimirResumoSolucao();
    cout << "\n=> SOLUCAO VALIDA! <=\n";

    double limite = calcularLimiteTeorico(r);
    if (isfinite(limite) && limite > 0.0) {
        cout << "\n--- Gap teorico da melhor solucao valida ---\n";
        cout << "Limite (sem capacidade): " << formatarNumero(limite) << " km\n";
        cout << "Solucao atual:           " << formatarNumero(atual) << " km\n";
        cout << "Gap absoluto:            " << formatarNumero(atual - limite) << " km\n";
        cout << "Gap relativo:            "
             << formatarNumero(100.0 * (atual - limite) / limite) << " %\n";
    } else {
        cout << "Gap teorico indisponivel.\n";
    }

    delete r;
}

ResultadoInstancia executarInstancia(const string& nomeArquivo,
                                     bool mostrarDetalhesMelhor) {
    ResultadoInstancia resultado;
    resultado.nome = nomeArquivo;

    cout << "\n============================================================\n";
    cout << "INSTANCIA: " << nomeArquivo << "\n";
    cout << "============================================================\n";

    Instancia* inst = lerInstancia(nomeArquivo);
    if (!inst) {
        cout << "Resultado: ERRO DE LEITURA; execucoes nao realizadas.\n";
        return resultado;
    }

    resultado.arquivoCarregado = true;
    cout << "Candidatos: " << inst->numCandidatos << "\n";
    cout << "Escolas:    " << inst->numLocais << "\n";
    cout << "Execucoes:  " << N_EXECUCOES << "\n";
    cout << "\n--- RESULTADOS POR EXECUCAO ---\n";

    vector<double> distanciasFinaisViaveis;
    distanciasFinaisViaveis.reserve(N_EXECUCOES);
    double somaTempos = 0.0;

    for (int run = 0; run < N_EXECUCOES; run++) {
        int seedAtual = run * 1000 + 42;
        double distInicial = 0.0;
        bool inicialViavel = false;
        bool finalViavel = false;

        auto t0 = chrono::steady_clock::now();
        resolvedor* r = rodarExecucao(inst, seedAtual, distInicial,
                                      inicialViavel, finalViavel);
        double distFinal = finalViavel ? r->avaliar() : 0.0;
        auto t1 = chrono::steady_clock::now();

        double tempo = chrono::duration<double>(t1 - t0).count();
        somaTempos += tempo;

        cout << "Execucao " << setw(2) << (run + 1) << "/" << N_EXECUCOES
             << " | seed: " << setw(6) << seedAtual
             << " | inicial: " << setw(13)
             << formatarMedida(inicialViavel, distInicial);
        if (inicialViavel) cout << " km";

        cout << " | final: " << setw(13)
             << formatarMedida(finalViavel, distFinal);
        if (finalViavel) cout << " km";

        cout << " | melhoria: " << setw(13)
             << formatarMedida(inicialViavel && finalViavel,
                               distInicial - distFinal);
        if (inicialViavel && finalViavel) cout << " km";

        cout << " | tempo: " << formatarNumero(tempo) << " s\n";

        if (finalViavel) {
            distanciasFinaisViaveis.push_back(distFinal);
            resultado.numViaveis++;

            if (resultado.numViaveis == 1 || distFinal < resultado.melhorDist) {
                resultado.melhorDist = distFinal;
                resultado.melhorSeed = seedAtual;
                resultado.distInicialMelhor = distInicial;
                resultado.inicialMelhorViavel = inicialViavel;
                resultado.melhoriaMelhor = inicialViavel
                    ? distInicial - distFinal : 0.0;

                resultado.limiteTeorico = calcularLimiteTeorico(r);
                if (isfinite(resultado.limiteTeorico) &&
                    resultado.limiteTeorico > 0.0) {
                    resultado.gapRelativo = 100.0 *
                        (distFinal - resultado.limiteTeorico) /
                        resultado.limiteTeorico;
                    resultado.gapDisponivel = true;
                } else {
                    resultado.gapDisponivel = false;
                }
            }
        }

        delete r;
    }

    resultado.tempoTotal = somaTempos;
    resultado.tempoMedio = somaTempos / N_EXECUCOES;

    if (!distanciasFinaisViaveis.empty()) {
        resultado.mediaDist = 0.0;
        for (double d : distanciasFinaisViaveis)
            resultado.mediaDist += d;
        resultado.mediaDist /= distanciasFinaisViaveis.size();

        resultado.piorDist = *max_element(distanciasFinaisViaveis.begin(),
                                           distanciasFinaisViaveis.end());

        double variancia = 0.0;
        for (double d : distanciasFinaisViaveis)
            variancia += (d - resultado.mediaDist) * (d - resultado.mediaDist);
        resultado.desvio = sqrt(variancia / distanciasFinaisViaveis.size());
        resultado.amplitude = resultado.piorDist - resultado.melhorDist;
    }

    imprimirMetricasInstancia(resultado);

    if (mostrarDetalhesMelhor)
        imprimirDetalhesMelhor(inst, resultado);

    liberarInstancia(inst);
    return resultado;
}

void imprimirResumoFinal(const vector<ResultadoInstancia>& resultados) {
    cout << "\n\n============================================================\n";
    cout << "RESUMO FINAL DAS INSTANCIAS\n";
    cout << "============================================================\n";
    cout << left
         << setw(22) << "Instancia" << " | "
         << setw(9)  << "Viaveis" << " | "
         << setw(14) << "Melhor final" << " | "
         << setw(13) << "Media final" << " | "
         << setw(12) << "Pior final" << " | "
         << setw(11) << "Desvio" << " | "
         << setw(12) << "Amplitude" << " | "
         << setw(14) << "Inicial melhor" << " | "
         << setw(12) << "Melhoria" << " | "
         << setw(9)  << "Gap (%)" << " | "
         << "Tempo medio (s)\n";
    cout << string(178, '-') << "\n";

    for (const ResultadoInstancia& r : resultados) {
        bool encontrou = r.arquivoCarregado && r.numViaveis > 0;
        bool inicialDisponivel = encontrou && r.inicialMelhorViavel;

        string viaveis;
        if (!r.arquivoCarregado) viaveis = "Erro leitura";
        else viaveis = to_string(r.numViaveis) + "/" + to_string(N_EXECUCOES);

        cout << left
             << setw(22) << r.nome << " | "
             << setw(9) << viaveis << " | "
             << setw(14) << (r.arquivoCarregado
                    ? formatarMedida(encontrou, r.melhorDist) : "Erro leitura") << " | "
             << setw(13) << (r.arquivoCarregado
                    ? formatarMedida(encontrou, r.mediaDist) : "Erro leitura") << " | "
             << setw(12) << (r.arquivoCarregado
                    ? formatarMedida(encontrou, r.piorDist) : "Erro leitura") << " | "
             << setw(11) << (r.arquivoCarregado
                    ? formatarMedida(encontrou, r.desvio) : "Erro leitura") << " | "
             << setw(12) << (r.arquivoCarregado
                    ? formatarMedida(encontrou, r.amplitude) : "Erro leitura") << " | "
             << setw(14) << (r.arquivoCarregado
                    ? formatarMedida(inicialDisponivel, r.distInicialMelhor) : "Erro leitura") << " | "
             << setw(12) << (r.arquivoCarregado
                    ? formatarMedida(inicialDisponivel, r.melhoriaMelhor) : "Erro leitura") << " | "
             << setw(9) << (r.arquivoCarregado
                    ? formatarMedida(encontrou && r.gapDisponivel,
                                     r.gapRelativo) : "Erro leitura") << " | "
             << (r.arquivoCarregado ? formatarNumero(r.tempoMedio) : "-")
             << "\n";
    }

    cout << "\nAs estatisticas de distancia consideram somente execucoes com solucao valida.\n";
    cout << "Inicial/Melhoria referem-se a execucao que obteve o melhor resultado final.\n";
    cout << "'Sem solucao' significa que nenhuma solucao valida foi encontrada nas 10 execucoes;\n";
    cout << "isso nao prova que a instancia seja matematicamente inviavel.\n";
}

int main() {
    vector<string> arquivos;
    for (int grupo = 1; grupo <= 2; grupo++) {
        for (int g = 1; g <= 14; g++) {
            arquivos.push_back("instancias/Instancia" + to_string(grupo) +
                               "_G" + to_string(g) + ".txt");
        }
    }

    cout << "Selecione a instancia para executar:\n";
    cout << "  0. Executar todas as 28 instancias\n";
    for (size_t i = 0; i < arquivos.size(); i++) {
        cout << setw(3) << (i + 1) << ". " << arquivos[i] << "\n";
    }
    cout << "Opcao: ";

    int opcao;
    if (!(cin >> opcao) || opcao < 0 || opcao > (int)arquivos.size()) {
        cerr << "Opcao invalida.\n";
        return 1;
    }

    vector<ResultadoInstancia> resultados;
    if (opcao == 0) {
        resultados.reserve(arquivos.size());
        for (const string& arquivo : arquivos) {
            // No modo todas, evita imprimir os detalhes completos da melhor solucao
            // de cada instancia; as metricas aparecem no resumo final.
            resultados.push_back(executarInstancia(arquivo, false));
        }
    } else {
        resultados.push_back(executarInstancia(arquivos[opcao - 1], true));
    }

    imprimirResumoFinal(resultados);
    return 0;
}
