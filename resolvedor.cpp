#include "resolvedor.h"
#include <algorithm>
#include <random>
#include <cmath>
#include <iostream>

resolvedor::resolvedor(float** matDist, int numCandidatos, int numLocais, int maxTipos){
    this->matDist = matDist;
    this->numCandidatos = numCandidatos;
    this->numLocais = numLocais;
    this->maxTipos = maxTipos;


    fracao_candidatos_tipo.assign(NUM_TIPOS, 0.0);

    for(int i=0; i < NUM_TIPOS; i++){
        escolasDoTipo[i] = 0;
    }
    for (int t = 0; t < NUM_TIPOS; t++) {
        candidatosPorTipo[t]      = 0;
        candidatosAlocadosTipo[t] = 0;
    }

    locaisProva = new LocalProva[numLocais];
    candidatos  = new Candidato[numCandidatos];



    // Para a busca local:


    melhorGanho = new Ganho**[numLocais];
    for(int i=0; i < numLocais; i++){
        melhorGanho[i] = new Ganho*[numLocais];
        for(int j=0; j<numLocais; j++)
        {
            melhorGanho[i][j] = new Ganho[NUM_TIPOS];
        }
    }





    int totalTrocas = numLocais * numLocais * maxTipos * maxTipos;
    trocasTipo = new double[totalTrocas];
    for (int k = 0; k < totalTrocas; k++) {
        trocasTipo[k] = -1e18;
    }



    int totalSum = numLocais * NUM_TIPOS * numLocais;
    sumDist = new double[totalSum];
    for (int k = 0; k < totalSum; k++) {
        sumDist[k] = 0.0;
    }




}

void resolvedor::resetarEstado() {
    for (int j = 0; j < numLocais; j++) {
        for (int s = 0; s < locaisProva[j].numSalas; s++) {
            locaisProva[j].tipoSalas[s] = TIPO_NENHUM;
            locaisProva[j].ocupSalas[s] = 0;
        }
        locaisProva[j].numTipos = 0;

        for (int t = 0; t < NUM_TIPOS; t++)
            locaisProva[j].alunosPorTipo[t] = 0;
        locaisProva[j].ocupTotal = 0;
    }
    for (int t = 0; t < NUM_TIPOS; t++) {
        escolasDoTipo[t]          = 0;
        candidatosAlocadosTipo[t] = 0;
    }
    for (int i = 0; i < numCandidatos; i++) {
        candidatos[i].escola = ESCOLA_NENHUMA;
        candidatos[i].sala   = SALA_NENHUMA;
    }
}


void resolvedor::construir(){

    std::vector<std::vector<int>> porTipo(NUM_TIPOS);
    for (int i = 0; i < numCandidatos; i++)
        porTipo[candidatos[i].tipo].push_back(i);


    int cap_maior = 0;
    for (int j = 0; j < numLocais; j++)
        cap_maior = std::max(cap_maior, locaisProva[j].capTotal);
    int k_amostra = std::max(1, cap_maior / NUM_TIPOS);

    for (int i = 0; i < NUM_TIPOS; i++){
        fracao_candidatos_tipo[i] = double(porTipo[i].size()) / numCandidatos;
        candidatosPorTipo[i] = porTipo[i].size();
    }
    mediaDist.assign(numLocais, std::vector<double>(NUM_TIPOS, 1e18));

    // Buffer reutilizado entre (escola, tipo)
    std::vector<float> buffer;
    buffer.reserve(numCandidatos);


    for (int j = 0; j < numLocais; j++) {
        for (int t = 0; t < NUM_TIPOS; t++) {
            const auto& lista = porTipo[t];
            if (lista.empty()) continue;

            int k = std::min(k_amostra, (int)lista.size());

            // Preenche buffer com as distâncias candidato escola j
            buffer.resize(lista.size());
            for (size_t idx = 0; idx < lista.size(); idx++)
                buffer[idx] = matDist[lista[idx]][j];

            // Particiona: buffer[0..k-1] são os k menores (em ordem qualquer)
            std::nth_element(buffer.begin(), buffer.begin() + k, buffer.end());

            // Media dos k menores
            double soma = 0.0;
            for (int idx = 0; idx < k; idx++)
                soma += buffer[idx];
            mediaDist[j][t] = soma / k;
        }
    }


    int maxTentativas = 120;
    int limitePuro = 30;
    bool viavel = false;
    int tentativas = 0;

    while (tentativas < maxTentativas) {
        resetarEstado();

        if (tentativas < 1*limitePuro/4) {
            fase1(false);              // roleta pura
        }
        else if(tentativas < limitePuro){
            fase1(true);
        }
        else {
            fase1Fallback();  // partição por capacidade

        }



        if (particaoObviamenteInviavel()) {
            viavel = false;
        } else {
            viavel = fase2();
        }

        tentativas++;
        seed += 50*tentativas;
        if (viavel) break;
    }
    if(!viavel)
        std::cerr << "solucao inviavel apos "<<tentativas<<" tentativas!" << std::endl;
    else
        std::cout << "solucao viavel apos " << tentativas <<" tentativas!"<< std::endl;


    int maximo = 0;
    for(int j=0; j<numLocais; j++)
    {
        if(locaisProva[j].capTotal > maximo){
            maximo = locaisProva[j].capTotal;
        }
    }

    melhoresCands = new int[maximo];
    for(int i=0; i< maximo; i++){
        melhoresCands[i] = -1;
    }

    ganhoTipos = new Ganho*[NUM_TIPOS];

    contadores = new int[NUM_TIPOS];

    for(int i=0; i<NUM_TIPOS; i++){
        contadores[i] = 0;

        ganhoTipos[i] = new Ganho[candidatosPorTipo[i]];
        for(int j=0; j<candidatosPorTipo[i]; j++){
            ganhoTipos[i][j] = {-1e18, -1};
        }
    }




}

double resolvedor::indicePreferencia(int j, int t){
    return 1.0 / mediaDist[j][t];

}

void resolvedor::calculoPontuacao(
    const std::vector<std::vector<double>>& indPref,
    std::vector<std::vector<double>>& pontuacoes)
{
    std::vector<std::vector<double>> preferenciasRelativas(
        NUM_TIPOS, std::vector<double>(numLocais, 0.0));

    for(int t = 0; t < NUM_TIPOS; t++){
        double acumuladoTipo = 0.0;
        for(int idx = 0; idx < numLocais; idx++){
            acumuladoTipo += indPref[t][idx];
        }
        if(acumuladoTipo <= 0.0) continue;

        for(int idx = 0; idx < numLocais; idx++){
            preferenciasRelativas[t][idx] = indPref[t][idx] / acumuladoTipo;
        }
    }

    for(int idx = 0; idx < numLocais; idx++){
        double totalLocal = 0.0;
        for(int t = 0; t < NUM_TIPOS; t++){
            totalLocal += preferenciasRelativas[t][idx];
        }
        if(totalLocal <= 0.0) continue;

        for(int t = 0; t < NUM_TIPOS; t++){
            double O = totalLocal - preferenciasRelativas[t][idx];
            if(O < 1e-9) O = 1e-9;
            pontuacoes[t][idx] = fracao_candidatos_tipo[t]
                               * preferenciasRelativas[t][idx] / O;
        }
    }


}

void resolvedor::calcularIndPref(std::vector<std::vector<double>>& indPref){
    for(int i = 0; i < NUM_TIPOS; i++){
        for(int j = 0; j < numLocais; j++){
            indPref[i][j] = indicePreferencia(j, i);
        }
    }


}

void resolvedor::fase1(bool forcarAncora){
    std::mt19937 gen(seed);

    int min_atrib = std::max(4, (int)std::ceil(0.4 * numLocais));
    int max_atrib = std::max(min_atrib, (int)std::floor(0.7 * numLocais));
    std::uniform_int_distribution<int> distAtrib(min_atrib, max_atrib);
    n_atribuicoes = distAtrib(gen);

    // ===== GARANTIA CONDICIONAL (só para instâncias apertadas) =====
    // Só aumenta o orçamento. Não chama roleta ainda.
    if (numLocais < 2 * NUM_TIPOS) {
        n_atribuicoes = std::max(n_atribuicoes, NUM_TIPOS);
    }

    std::vector<std::vector<double>> indPref(
        NUM_TIPOS, std::vector<double>(numLocais, 0.0));
    calcularIndPref(indPref);
    std::vector<std::vector<double>> pontuacoes(
        NUM_TIPOS, std::vector<double>(numLocais, 0.0));
    calculoPontuacao(indPref, pontuacoes);

    int restantes = n_atribuicoes;

    // ===== ROLETA PRINCIPAL (inalterada) =====
    for(int iter = 0; iter < restantes; iter++){
        std::vector<std::pair<int, int>> pares;
        std::vector<double> pesos;
        double soma = 0.0;

        for(int t = 0; t < NUM_TIPOS; t++){
            if(escolasDoTipo[t] >= MAX_ESCOLAS_FASE1_TIPO) continue;
            for(int j = 0; j < numLocais; j++){
                if(locaisProva[j].numTipos > 0) continue;
                if(locaisProva[j].temTipo(t)) continue;
                double p = pontuacoes[t][j];
                if(p <= 0.0) continue;
                pares.push_back({t, j});
                pesos.push_back(p);
                soma += p;
            }
        }
        if(soma <= 0.0) break;

        std::uniform_real_distribution<double> distrib(0.0, soma);
        double numAleatorio = distrib(gen);
        double acumulado = 0.0;
        for(size_t k = 0; k < pares.size(); k++){
            acumulado += pesos[k];
            if(acumulado > numAleatorio){
                int t = pares[k].first;
                int j = pares[k].second;
                locaisProva[j].adicionarTipo(t, SALAS_FASE1);
                escolasDoTipo[t]++;
                break;
            }
        }
    }

    // ===== REDE DE SEGURANÇA (só para instâncias apertadas) =====
    // Roda DEPOIS da roleta. Só atua em tipos que ficaram com 0 âncora.
    if (numLocais < 2 * NUM_TIPOS) {
        for (int t = 0; t < NUM_TIPOS; t++) {
            if (escolasDoTipo[t] > 0) continue;
            if (candidatosPorTipo[t] == 0) continue;

            int melhor_j = -1;
            double melhor_d = 1e18;
            for (int j = 0; j < numLocais; j++) {
                if (locaisProva[j].numTipos >= MAX_TIPOS_ESCOLA) continue;
                if (locaisProva[j].temTipo(t)) continue;
                int cap_livre = 0;
                for (int s = 0; s < locaisProva[j].numSalas; s++)
                    if (locaisProva[j].tipoSalas[s] == TIPO_NENHUM)
                        cap_livre += locaisProva[j].capSalas[s];
                if (cap_livre < 30) continue;
                if (mediaDist[j][t] < melhor_d) {
                    melhor_d = mediaDist[j][t];
                    melhor_j = j;
                }
            }
            if (melhor_j != -1) {
                locaisProva[melhor_j].adicionarTipo(t, SALAS_FASE1);
                escolasDoTipo[t]++;
            }
        }
    }
}

bool resolvedor::fase1Fallback() {
    std::mt19937 gen(seed);

    std::vector<double> chave(numLocais);
    std::uniform_real_distribution<double> jit(0.95, 1.05);
    for (int j = 0; j < numLocais; j++)
        chave[j] = locaisProva[j].capTotal * jit(gen);

    std::vector<int> escolas(numLocais);
    for (int j = 0; j < numLocais; j++) escolas[j] = j;
    std::sort(escolas.begin(), escolas.end(), [&](int a, int b) {
        return chave[a] > chave[b];
    });

    std::vector<int> remaining(NUM_TIPOS);
    for (int t = 0; t < NUM_TIPOS; t++)
        remaining[t] = candidatosPorTipo[t];

    for (int s = 0; s < numLocais; s++) {
        int j = escolas[s];
        int max_t = -1, max_rem = 0;
        for (int t = 0; t < NUM_TIPOS; t++) {
            if (remaining[t] > max_rem) { max_rem = remaining[t]; max_t = t; }
        }
        if (max_t == -1) break;
        int salas_reservar = std::max(1, locaisProva[j].numSalas - 1);
        locaisProva[j].adicionarTipo(max_t, salas_reservar);
        escolasDoTipo[max_t]++;
        remaining[max_t] -= locaisProva[j].capTotal;
    }
    return true;
}

double resolvedor::aglomeracao(int j, int t){
    return 1.0 + std::min(
        ((double)locaisProva[j].alunosPorTipo[t] / locaisProva[j].capTotal)
            * BIAS_REL,
        TETO_AGLOM - 1.0);


}

double resolvedor::disponibilidade(int j, int t){
    if (!locaisProva[j].podeReceber(t))
        return 0.0;


    // 1) Vagas utilizáveis pelo tipo t:
    //    - vagas em salas ainda sem tipo (que podem virar t)
    //    - vagas livres em salas já do tipo t (se a escola tiver t)
    int vagas_util = 0;
    for (int s = 0; s < locaisProva[j].numSalas; s++) {
        if (locaisProva[j].tipoSalas[s] == TIPO_NENHUM) {
            vagas_util += locaisProva[j].capSalas[s];
        }
        else if (locaisProva[j].tipoSalas[s] == t) {
            vagas_util += locaisProva[j].capSalas[s] - locaisProva[j].ocupSalas[s];
        }
    }

    if (vagas_util <= 0)
        return 0.0;

    double cap_total = (double)locaisProva[j].capTotal;

    double fatorAmplTipo = 1.0;
    if(locaisProva[j].temTipo(t))
        fatorAmplTipo = 2.0;

    double fator = BASE_DISP + (1.0 - BASE_DISP) * vagas_util / cap_total;

    // 2) Penalização se a escola já está comprometida com outro tipo
    //    (só faz sentido quando t ainda não está presente em j)
    if (!locaisProva[j].temTipo(t)) {
        int cap_outro = 0;
        for (int s = 0; s < locaisProva[j].numSalas; s++) {
            int ts = locaisProva[j].tipoSalas[s];
            if (ts != TIPO_NENHUM && ts != t)
                cap_outro += locaisProva[j].capSalas[s];
        }
        double carga_outro = (double)cap_outro / cap_total;
        fator *= (1.0 - carga_outro / 2.0);
    }

    return std::max(fatorAmplTipo * fator, 0.0);
}



double resolvedor::pontuacaoAresta(int c, int j, int t){
    double fatorAglomeracao = aglomeracao(j, t);
    double fatorDisponibilidade = disponibilidade(j, t);

    if (fatorDisponibilidade == 0.0)
        return 1e18;

    return matDist[c][j] * fatorAglomeracao / fatorDisponibilidade;
}


bool resolvedor::verificaViabilidadeAtual() {
    int vagasTotaisLivres = 0;
    int deficitTotal = 0;

    for (int t = 0; t < NUM_TIPOS; t++) {
        int restantes_t = candidatosPorTipo[t] - candidatosAlocadosTipo[t];
        if (restantes_t <= 0) continue;

        int exclusiva_t = 0;
        for (int j = 0; j < numLocais; j++)
            exclusiva_t += locaisProva[j].vagasLivresTipo(t);

        if (restantes_t > exclusiva_t)
            deficitTotal += (restantes_t - exclusiva_t);
    }

    for (int j = 0; j < numLocais; j++) {
        for (int s = 0; s < locaisProva[j].numSalas; s++) {
            if (locaisProva[j].tipoSalas[s] == TIPO_NENHUM)
                vagasTotaisLivres += locaisProva[j].capSalas[s];
        }
    }

    return deficitTotal <= vagasTotaisLivres;
}


void resolvedor::montarTop3(std::vector<std::array<Aresta, 3>> &top3, int i){
        for(int j=0; j <numLocais; j++){
            double pontuacaoArestaCalculada = pontuacaoAresta(i, j, candidatos[i].tipo);

            arestaValida[j][i] = (pontuacaoArestaCalculada < 1e17);
            if(arestaValida[j][i]) totalArestas[i] ++;

            if(pontuacaoArestaCalculada < top3[i][0].pontuacao){
                top3[i][2] = top3[i][1];
                top3[i][1] = top3[i][0];
                top3[i][0] = {pontuacaoArestaCalculada, j};
            }
            else if(pontuacaoArestaCalculada < top3[i][1].pontuacao){
                top3[i][2] = top3[i][1];
                top3[i][1] = {pontuacaoArestaCalculada, j};
            }
            else if(pontuacaoArestaCalculada < top3[i][2].pontuacao){
                top3[i][2] = {pontuacaoArestaCalculada, j};
            }

        }

}


int resolvedor::primeiroCritico(const std::vector<std::array<Aresta, 3>> &top3){

    for(int i=0; i<numCandidatos; i++){
            if(candidatos[i].escola != -1)
                continue;
            if(top3[i][2].pontuacao > 1e17){
                return i;
            }

    }

    return -1;

}






void resolvedor::atualizarArestas(int j, int tipoAlocado, bool atualizarTodos, std::set<std::pair<int, int>>& fila) {


    for(int k = 0; k < numCandidatos; k++){
        if(candidatos[k].escola != ESCOLA_NENHUMA) continue;
        if (!atualizarTodos && candidatos[k].tipo != tipoAlocado) continue;

        bool eraValido = arestaValida[k][j];
        bool ehValido = (pontuacaoAresta(k, j, candidatos[k].tipo) < 1e17);

        // Se o estado da aresta mudou (encheu ou abriu vaga)
        if (eraValido != ehValido) {
            // Remove o candidato da fila com o valor antigo
            fila.erase({totalArestas[k], k});

            // Atualiza os contadores
            if (ehValido) {
                totalArestas[k]++;
                arestaValida[k][j] = true;
            } else {
                totalArestas[k]--;
                arestaValida[k][j] = false;
            }

            // Reinsere o candidato na fila com o novo valor
            fila.insert({totalArestas[k], k});
        }
    }
}



bool resolvedor::particaoObviamenteInviavel(){
    // 1. Algum tipo sem âncora E sem escolas disponíveis para abrir?
    int sem_ancora = 0;
    for (int t = 0; t < NUM_TIPOS; t++)
        if (candidatosPorTipo[t] > 0 && escolasDoTipo[t] == 0)
            sem_ancora++;

    int aberturas = 0;
    for (int j = 0; j < numLocais; j++) {
        if (locaisProva[j].numTipos >= MAX_TIPOS_ESCOLA) continue;
        for (int s = 0; s < locaisProva[j].numSalas; s++)
            if (locaisProva[j].tipoSalas[s] == TIPO_NENHUM) {
                aberturas++;
                break;   // conta cada escola só uma vez
            }
    }
    if (sem_ancora > aberturas) return true;

    // 2. Capacidade por tipo com cota de pool
    int pool_livre = 0;
    for (int j = 0; j < numLocais; j++)
        for (int s = 0; s < locaisProva[j].numSalas; s++)
            if (locaisProva[j].tipoSalas[s] == TIPO_NENHUM)
                pool_livre += locaisProva[j].capSalas[s];

    int cota = (sem_ancora > 0) ? pool_livre / sem_ancora : pool_livre / NUM_TIPOS;

    for (int t = 0; t < NUM_TIPOS; t++) {
        int precisa = candidatosPorTipo[t];
        if (precisa == 0) continue;

        int exclusiva = 0;
        for (int j = 0; j < numLocais; j++)
            if (locaisProva[j].temTipo(t))
                exclusiva += locaisProva[j].capTotal;

        if (precisa > exclusiva + cota) return true;
    }
    return false;
}

bool resolvedor::fase2(){
    std::mt19937 gen(seed);

    arestaValida.assign(numCandidatos, std::vector<bool>(numLocais, false));
    totalArestas.assign(numCandidatos, 0);
    std::set<std::pair<int, int>> fila;

    // Inicialização: preenche a matriz e a fila
    std::vector<int> ids(numCandidatos);
    for(int i = 0; i < numCandidatos; i++) ids[i] = i;
    std::shuffle(ids.begin(), ids.end(), gen);

    // Inicialização: preenche a matriz e a fila
    for(int i : ids){ // Usa a ordem embaralhada
        for(int j = 0; j < numLocais; j++){
            if(pontuacaoAresta(i, j, candidatos[i].tipo) < 1e17){
                arestaValida[i][j] = true;
                totalArestas[i]++;
            }
        }
        fila.insert({totalArestas[i], i});
    }

    int restantes = numCandidatos;

    while(restantes != 0){
        // Pega o candidato com MENOS opções
        auto it = fila.begin();
        int c = it->second;

        if (it->first == 0) {
            //std::cerr << "Instância inviavel: candidato " << c << " sem opções.\n";
            return false;
        }

        // Calcula o "Top 3" do candidato c NA HORA (on-demand)
        std::vector<std::pair<double, int>> top3_local;
        for(int j = 0; j < numLocais; j++){
            if(arestaValida[c][j]){
                double p = pontuacaoAresta(c, j, candidatos[c].tipo);
                top3_local.push_back({p, j});
            }
        }
        // Ordena as opções calculadas
        std::sort(top3_local.begin(), top3_local.end());

        // Pega as 3 melhores (ou menos, se houver menos de 3)
        int num_opcoes = std::min(3, (int)top3_local.size());
        double melhor = top3_local[0].first;

        std::vector<int> rcl;
        for(int k = 0; k < num_opcoes; k++){
            if(top3_local[k].first <= melhor * (1.0 + ALPHA)){
                rcl.push_back(top3_local[k].second); // guarda o índice da escola
            }
        }

        // Opcional: embaralhar a RCL
        std::shuffle(rcl.begin(), rcl.end(), gen);

        int t = candidatos[c].tipo;
        bool alocou = false;

        // 1. Tenta primeiro a RCL (variabilidade do GRASP)
        for(int j : rcl){
            bool tipoCriado = !locaisProva[j].temTipo(t);

            // Etapa checagens
            int salas_livres_antes = 0;
            for (int s = 0; s < locaisProva[j].numSalas; s++)
                if (locaisProva[j].tipoSalas[s] == TIPO_NENHUM) salas_livres_antes++;
            //

            int sala = locaisProva[j].alocarCandidato(t);

            // Checagens 2
                        int salas_livres_depois = 0;
            for (int s = 0; s < locaisProva[j].numSalas; s++)
                if (locaisProva[j].tipoSalas[s] == TIPO_NENHUM) salas_livres_depois++;

            bool nova_sala_aberta = (salas_livres_depois < salas_livres_antes);

            // ===== NOVO: guard contra -1 =====
            if (sala == -1) {
                std::cerr << "[BUG] fase2/RCL: alocarCandidato falhou | cand=" << c
                          << " tipo=" << t << " escola=" << j
                          << " numTipos=" << locaisProva[j].numTipos << "\n";
                continue;   // ou exit(1) para abortar
            }

            candidatos[c].escola = j;
            candidatos[c].sala = sala;

            if(verificaViabilidadeAtual()){
                candidatosAlocadosTipo[t]++;
                restantes--;
                fila.erase(it);
                atualizarArestas(j, t, nova_sala_aberta, fila);
                alocou = true;
                break;
            } else {
                // Desfaz
                locaisProva[j].desalocarCandidato(t, sala);
                if(tipoCriado) locaisProva[j].retirarTipo(t);
                candidatos[c].escola = ESCOLA_NENHUMA;
                candidatos[c].sala = SALA_NENHUMA;
            }
        }

        // 2. FALLBACK: Se a RCL falhou, tenta o resto das escolas válidas (guloso puro)
        if(!alocou) {
            for(int idx = 0; idx < (int)top3_local.size(); idx++){
                int j = top3_local[idx].second;

                // Pula as escolas que já foram testadas na RCL
                bool jaTestou = false;
                for(int r : rcl) {
                    if(r == j) { jaTestou = true; break; }
                }
                if(jaTestou) continue;

                bool tipoCriado = !locaisProva[j].temTipo(t);

                //
                int salas_livres_antes = 0;
                for (int s = 0; s < locaisProva[j].numSalas; s++)
                    if (locaisProva[j].tipoSalas[s] == TIPO_NENHUM) salas_livres_antes++;

                //
                int sala = locaisProva[j].alocarCandidato(t);

                //
                int salas_livres_depois = 0;
                for (int s = 0; s < locaisProva[j].numSalas; s++)
                    if (locaisProva[j].tipoSalas[s] == TIPO_NENHUM) salas_livres_depois++;

                bool nova_sala_aberta = (salas_livres_depois < salas_livres_antes);

                if (sala == -1) {
                    std::cerr << "[BUG] fase2/fallback: alocarCandidato falhou | cand=" << c
                              << " tipo=" << t << " escola=" << j << "\n";
                    continue;
                }
                //

                candidatos[c].escola = j;
                candidatos[c].sala = sala;

                if(verificaViabilidadeAtual()){
                    candidatosAlocadosTipo[t]++;
                    restantes--;
                    fila.erase(it);
                    atualizarArestas(j, t, nova_sala_aberta, fila);
                    alocou = true;
                    break;
                } else {
                    locaisProva[j].desalocarCandidato(t, sala);
                    if(tipoCriado) locaisProva[j].retirarTipo(t);
                    candidatos[c].escola = ESCOLA_NENHUMA;
                    candidatos[c].sala = SALA_NENHUMA;
                }
            }
        }

        if(!alocou) {
            // Agora sim, tentou TODAS as escolas válidas e nenhuma manteve a viabilidade.
            return false;
        }
    }
    return true;
}


float resolvedor::avaliar(){
    float total = 0.0f;
    for (int i = 0; i < numCandidatos; i++) {
        if (candidatos[i].escola == ESCOLA_NENHUMA) continue;
        total += matDist[i][candidatos[i].escola];
    }
    return total;
}


resolvedor::~resolvedor() {
    delete[] locaisProva;
    delete[] candidatos;
    for(int i=0; i < numLocais; i++){

        for(int j=0; j<numLocais; j++)
        {
            delete [] melhorGanho[i][j] ;
        }
        delete [] melhorGanho[i];
    }
    delete [] melhorGanho;

    for(int i=0; i < NUM_TIPOS; i++){
        delete [] ganhoTipos[i];
    }

    delete [] ganhoTipos;

    delete [] contadores;

    delete [] melhoresCands;

    delete[] trocasTipo;

    delete[] sumDist;


    // matDist: se o resolvedor é dono, precisa delete[] em cada linha e no vetor.
    // Se veio de fora, NÃO delete.
}


bool resolvedor::buscaLocal_MoverCandidato(int K) {

    std::vector<Aluno> alunos;
    alunos.reserve(numCandidatos);

    bool melhora = false;

    for (int i = 0; i < numCandidatos; i++) {
        if (candidatos[i].escola != ESCOLA_NENHUMA) {
            double d = matDist[i][candidatos[i].escola];
            alunos.push_back({d, i});
        }
    }
    int limite = std::min(K, (int)alunos.size());

    // 2. Ordena do pior (maior distância) para o melhor
    std::nth_element(alunos.begin(), alunos.begin() + limite, alunos.end(),
        [](const Aluno& a, const Aluno& b) {
            return a.distancia > b.distancia; // Pior (maior distância) primeiro
        });


    // 3. Ordena os K maiores
    std::sort(alunos.begin(), alunos.begin()+limite, [](const Aluno& a, const Aluno &b){
              return a.distancia > b.distancia; });

    // 4. Tenta mover cada um deles

    for(int i=0; i < limite; i++){

        Candidato* cand = candidatos + alunos[i].id_cand;
        int localEscolhido = cand->escola;

        for(int j=0; j < numLocais; j++){
            if(locaisProva[j].temTipo(cand->tipo) && matDist[alunos[i].id_cand][j] < alunos[i].distancia && locaisProva[j].podeReceber(cand->tipo))
            {
                localEscolhido = j;
                alunos[i].distancia = matDist[alunos[i].id_cand][j];
            }
        }

        if(localEscolhido != cand->escola){

            locaisProva[cand->escola].desalocarCandidato(cand->tipo, cand->sala);

            int sala = locaisProva[localEscolhido].alocarCandidato(cand->tipo);
            if(sala == -1){
                std::cerr << "Incongruencia entre as funcoes \"podeRebecer\" e \"alocarCandidato\". "<<std::endl;
                exit(1);
            }
            cand->escola = localEscolhido;
            cand->sala = sala;

            melhora = true;

        }


    }


    for (int j = 0; j < numLocais; j++) {
        for (int idx = locaisProva[j].numTipos - 1; idx >= 0; idx--) {
            int t = locaisProva[j].tipos[idx];
            if (locaisProva[j].alunosPorTipo[t] == 0) {
                locaisProva[j].retirarTipo(t);
            }
        }
    }

    // Se testou os K alunos e não achou nenhum movimento que melhorasse
    return melhora;
}



void resolvedor::construirTabelaGanhos() {
    // 1. Inicializa tudo vazio
    for (int j = 0; j < numLocais; j++)
        for (int i = 0; i < numLocais; i++)
            for (int t = 0; t < NUM_TIPOS; t++)
                melhorGanho[j][i][t] = { -1e18, -1 };

    // 2. Para cada candidato, avalia todos os destinos possíveis
    for (int c = 0; c < numCandidatos; c++) {
        if (candidatos[c].escola == ESCOLA_NENHUMA) continue;
        int j = candidatos[c].escola;
        int t = candidatos[c].tipo;

        for (int i = 0; i < numLocais; i++) {
            if (i == j) continue;
            if (!locaisProva[i].temTipo(t)) continue;  // i não serve t

            double g = matDist[c][j] - matDist[c][i];
            if (g > melhorGanho[j][i][t].valor) {
                melhorGanho[j][i][t] = { g, c };
            }
        }
    }
}

void resolvedor::atualizarTabelaGanhos(int X, int Y, int t, int A, int B) {
    // ===== Linha X: A saiu, B chegou =====
    for (int i = 0; i < numLocais; i++) {
        if (i == X) continue;
        if (!locaisProva[i].temTipo(t)) continue;

        if (melhorGanho[X][i][t].id_cand == A) {
            // A era o topo  rescan completo de X
            melhorGanho[X][i][t] = { -1e18, -1 };
            for (int c = 0; c < numCandidatos; c++) {
                if (candidatos[c].escola != X || candidatos[c].tipo != t) continue;
                double g = matDist[c][X] - matDist[c][i];
                if (g > melhorGanho[X][i][t].valor)
                    melhorGanho[X][i][t] = { g, c };
            }
        } else {
            // Topo ainda está em X. Só checa se B (novo) supera
            double gB = matDist[B][X] - matDist[B][i];
            if (gB > melhorGanho[X][i][t].valor)
                melhorGanho[X][i][t] = { gB, B };
        }
    }
    // ===== Linha Y: B saiu, A chegou (análogo) =====
    for (int i = 0; i < numLocais; i++) {
        if (i == Y) continue;
        if (!locaisProva[i].temTipo(t)) continue;

        if (melhorGanho[Y][i][t].id_cand == B) {
            melhorGanho[Y][i][t] = { -1e18, -1 };
            for (int c = 0; c < numCandidatos; c++) {
                if (candidatos[c].escola != Y || candidatos[c].tipo != t) continue;
                double g = matDist[c][Y] - matDist[c][i];
                if (g > melhorGanho[Y][i][t].valor)
                    melhorGanho[Y][i][t] = { g, c };
            }
        } else {
            double gA = matDist[A][Y] - matDist[A][i];
            if (gA > melhorGanho[Y][i][t].valor)
                melhorGanho[Y][i][t] = { gA, A };
        }
    }
}

bool resolvedor::buscaLocal_TrocarCandidatos(){



    double melhorDelta = 0.0;
    int melhorX = -1, melhorY = -1, melhorT = -1;
    int melhorA = -1, melhorB = -1;

    for(int i=0; i<numLocais; i++){
        for(int j=i+1; j<numLocais;j++){
            for(int t=0; t<NUM_TIPOS; t++){
                double delta = melhorGanho[i][j][t].valor + melhorGanho[j][i][t].valor;
                if(delta > melhorDelta){
                    melhorDelta = delta;
                    melhorX = i;
                    melhorY = j;
                    melhorT = t;
                    melhorA = melhorGanho[i][j][t].id_cand;
                    melhorB = melhorGanho[j][i][t].id_cand;
                }
            }
        }
    }

    if(melhorX == -1)
        return false;

    locaisProva[melhorX].desalocarCandidato(melhorT, candidatos[melhorA].sala);
    locaisProva[melhorY].desalocarCandidato(melhorT, candidatos[melhorB].sala);

    int sala = locaisProva[melhorX].alocarCandidato(melhorT);
    candidatos[melhorB].escola = melhorX;
    candidatos[melhorB].sala = sala;

    sala = locaisProva[melhorY].alocarCandidato(melhorT);
    candidatos[melhorA].escola = melhorY;
    candidatos[melhorA].sala = sala;

    atualizarTabelaGanhos(melhorX, melhorY, melhorT, melhorA, melhorB);

    return true;

}


bool resolvedor::buscaLocal_AlocarTipo() {
    double maximoGanho = 0.0;
    int melhorX = -1, melhorT = -1, numDeslocados = 0;

    for (int X = 0; X < numLocais; X++) {
        if (locaisProva[X].numTipos >= maxTipos) continue;

        int vagasReais = 0;
        for (int s = 0; s < locaisProva[X].numSalas; s++)
            if (locaisProva[X].tipoSalas[s] == -1)
                vagasReais += locaisProva[X].capSalas[s];
        if (vagasReais == 0) continue;

        // Reset contadores para a escola X
        for (int t = 0; t < NUM_TIPOS; t++) contadores[t] = 0;

        // Preenche ganhos: candidatos de tipo t que podem ir para X
        for (int c = 0; c < numCandidatos; c++) {
            if (candidatos[c].escola == ESCOLA_NENHUMA) continue;
            int t = candidatos[c].tipo;
            if (locaisProva[X].temTipo(t)) continue;
            double g = matDist[c][candidatos[c].escola] - matDist[c][X];
            ganhoTipos[t][contadores[t]++] = { g, c };
        }

        for (int t = 0; t < NUM_TIPOS; t++) {
            if (locaisProva[X].temTipo(t)) continue;
            if (contadores[t] == 0) continue;

            int k = std::min(vagasReais, contadores[t]);
            std::nth_element(ganhoTipos[t], ganhoTipos[t] + k, ganhoTipos[t] + contadores[t],
                [](const Ganho& a, const Ganho& b){ return a.valor > b.valor; });

            double total = 0.0;
            for (int i = 0; i < k; i++) total += ganhoTipos[t][i].valor;

            if (total > maximoGanho) {
                maximoGanho = total;
                melhorX = X;
                melhorT = t;
                numDeslocados = k;
                for (int i = 0; i < k; i++)
                    melhoresCands[i] = ganhoTipos[t][i].id_cand;
            }
        }
    }

    if (melhorT == -1) return false;

    // Executa
    if (!locaisProva[melhorX].adicionarTipo(melhorT, 1))
        return false;

    for (int i = 0; i < numDeslocados; i++) {
        int c = melhoresCands[i];
        int j_old = candidatos[c].escola;
        int sala_old = candidatos[c].sala;
        locaisProva[j_old].desalocarCandidato(melhorT, sala_old);
        int nova_sala = locaisProva[melhorX].alocarCandidato(melhorT);
        if (nova_sala == -1) {
            std::cerr << "[BUG] AlocarTipo: alocacao falhou\n";
            exit(1);
        }
        candidatos[c].escola = melhorX;
        candidatos[c].sala = nova_sala;
    }

    for (int j = 0; j < numLocais; j++) {
        for (int idx = locaisProva[j].numTipos - 1; idx >= 0; idx--) {
            int t = locaisProva[j].tipos[idx];
            if (locaisProva[j].alunosPorTipo[t] == 0) {
                locaisProva[j].retirarTipo(t);
            }
        }
    }

    return true;
}

void resolvedor::construirSumDist() {
    int total = numLocais * NUM_TIPOS * numLocais;
    for (int k = 0; k < total; k++) {
        sumDist[k] = 0.0;
    }

    for (int c = 0; c < numCandidatos; c++) {
        if (candidatos[c].escola == ESCOLA_NENHUMA) continue;

        int X = candidatos[c].escola;
        int t = candidatos[c].tipo;

        for (int Y = 0; Y < numLocais; Y++) {
            sumDist[idxSum(X, t, Y)] += matDist[c][Y];
        }
    }
}



void resolvedor::construirTrocasTipo() {
    construirSumDist();   // rebuild completo

    int totalTrocas = numLocais * numLocais * maxTipos * maxTipos;
    for (int k = 0; k < totalTrocas; k++) {
        trocasTipo[k] = -1e18;
    }

    for (int i = 0; i < numLocais; i++) {
        for (int j = i + 1; j < numLocais; j++) {
            for (int a = 0; a < locaisProva[i].numTipos; a++) {
                for (int b = 0; b < locaisProva[j].numTipos; b++) {
                    int ti = locaisProva[i].tipos[a];
                    int tj = locaisProva[j].tipos[b];

                    if (ti == tj) continue;
                    if (locaisProva[j].temTipo(ti)) continue;
                    if (locaisProva[i].temTipo(tj)) continue;

                    // Checagem de capacidade (a sua versão, que está correta)
                    int espacoI = locaisProva[i].alunosPorTipo[ti]
                                + locaisProva[i].vagasLivresPotenciaisTipo(ti);
                    int espacoJ = locaisProva[j].alunosPorTipo[tj]
                                + locaisProva[j].vagasLivresPotenciaisTipo(tj);

                    if (espacoI < locaisProva[j].alunosPorTipo[tj]) continue;
                    if (espacoJ < locaisProva[i].alunosPorTipo[ti]) continue;

                    // Delta O(1) via tabela plana
                    double deltaI = sumDist[idxSum(i, ti, i)]
                                  - sumDist[idxSum(i, ti, j)];
                    double deltaJ = sumDist[idxSum(j, tj, j)]
                                  - sumDist[idxSum(j, tj, i)];

                    trocasTipo[idxTroca(i, j, a, b)] = deltaI + deltaJ;
                }
            }
        }
    }
}



bool resolvedor::buscaLocal_MudarTipo_1_1() {
    construirTrocasTipo();

    double melhorDelta = 0.0;
    int melhorI = -1, melhorJ = -1, melhorA = -1, melhorB = -1;

    for (int i = 0; i < numLocais; i++) {
        for (int j = i + 1; j < numLocais; j++) {
            for (int a = 0; a < locaisProva[i].numTipos; a++) {
                for (int b = 0; b < locaisProva[j].numTipos; b++) {
                    double d = trocasTipo[idxTroca(i, j, a, b)];
                    if (d > melhorDelta) {
                        melhorDelta = d;
                        melhorI = i; melhorJ = j;
                        melhorA = a; melhorB = b;
                    }
                }
            }
        }
    }

    if (melhorI == -1) return false;

    int tipo1 = locaisProva[melhorI].tipos[melhorA];
    int tipo2 = locaisProva[melhorJ].tipos[melhorB];

    locaisProva[melhorI].retirarTipo(tipo1);
    locaisProva[melhorJ].retirarTipo(tipo2);

    for (int c = 0; c < numCandidatos; c++) {
        if (candidatos[c].escola == melhorI && candidatos[c].tipo == tipo1) {
            candidatos[c].sala   = locaisProva[melhorJ].alocarCandidato(tipo1);
            candidatos[c].escola = melhorJ;
        }
        else if (candidatos[c].escola == melhorJ && candidatos[c].tipo == tipo2) {
            candidatos[c].sala   = locaisProva[melhorI].alocarCandidato(tipo2);
            candidatos[c].escola = melhorI;
        }
    }

    return true;
}

bool resolvedor::buscaLocal(int K) {
    double distInicial = avaliar();
    bool solucaoInicialValida = verificarSolucao(false);
    double distAnterior = distInicial;

    const double EPS_KM = 5.0;


    int totalMover = 0, totalSwap = 0, totalAlocar = 0, totalMudarTipo = 0;
    int ciclo = 0;

    while (true) {
        ciclo++;
        bool progresso = false;

        // Fase 1: Mover
        int iterMover = 0;
        while (buscaLocal_MoverCandidato(K)) { iterMover++; progresso = true; }
        totalMover += iterMover;
        double distPosMover = avaliar();

        // Fase 2: Swap
        construirTabelaGanhos();
        int iterSwap = 0;
        while (buscaLocal_TrocarCandidatos()) { iterSwap++; progresso = true; }
        totalSwap += iterSwap;
        double distPosSwap = avaliar();

        // Fase 3: AlocarTipo (1 por ciclo)
        bool alocou = buscaLocal_AlocarTipo();
        if (alocou) { totalAlocar++; progresso = true; }
        double distPosAlocar = avaliar();

        // Fase 4: MudarTipo 1-1 (1 por ciclo, mais cara e estrutural)
        bool mudouTipo = buscaLocal_MudarTipo_1_1();
        if (mudouTipo) { totalMudarTipo++; progresso = true; }
        double distPosMudar = avaliar();

        // Log do ciclo
        std::cout << "--- Ciclo " << ciclo << " ---\n";
        if (iterMover > 0)
            std::cout << "  Mover:        " << iterMover
                      << " iter | dist=" << distPosMover
                      << " | ganho=" << (distAnterior - distPosMover) << " km\n";
        if (iterSwap > 0)
            std::cout << "  Swap:         " << iterSwap
                      << " iter | dist=" << distPosSwap
                      << " | ganho=" << (distPosMover - distPosSwap) << " km\n";
        if (alocou)
            std::cout << "  AlocarTipo:   1 tipo  | dist=" << distPosAlocar
                      << " | ganho=" << (distPosSwap - distPosAlocar) << " km\n";
        if (mudouTipo)
            std::cout << "  MudarTipo1-1: 1 troca | dist=" << distPosMudar
                      << " | ganho=" << (distPosAlocar - distPosMudar) << " km\n";



        double ganhoCiclo = distAnterior - distPosMudar;

        if (ganhoCiclo < EPS_KM) {
            std::cout << "[Convergencia] Ganho " << ganhoCiclo
                      << " < " << EPS_KM << " km, parando.\n";
            break;
        }


        distAnterior = distPosMudar;

        if (!progresso) break;



        // Guarda de segurança (remova apos validar)
        if (ciclo > 30) {
            std::cerr << "[AVISO] VND nao convergiu em 30 ciclos\n";
            break;
        }
    }

    double distFinal = avaliar();
    bool solucaoFinalValida = verificarSolucao();

    std::cout << "\n=== RESUMO BUSCA LOCAL ===\n"
              << "Ciclos VND:        " << ciclo << "\n"
              << "Iter. Mover:       " << totalMover << "\n"
              << "Iter. Swap:        " << totalSwap << "\n"
              << "Tipos alocados:    " << totalAlocar << "\n"
              << "Trocas de tipo:    " << totalMudarTipo << "\n";

    if (solucaoInicialValida)
        std::cout << "Dist. inicial:     " << distInicial << " km\n";
    else
        std::cout << "Dist. inicial:     Sem solucao\n";

    if (solucaoFinalValida)
        std::cout << "Dist. final:       " << distFinal << " km\n";
    else
        std::cout << "Dist. final:       Sem solucao\n";

    if (solucaoInicialValida && solucaoFinalValida) {
        double ganhoTotal = distInicial - distFinal;
        double pctTotal = (distInicial != 0.0) ? 100.0 * ganhoTotal / distInicial : 0.0;
        std::cout << "Ganho total:       " << ganhoTotal << " km (" << pctTotal << "%)\n";
    } else {
        std::cout << "Ganho total:       Sem solucao\n";
    }

    if (!solucaoFinalValida)
        std::cerr << "[FALHA] Solucao invalida apos busca local!\n";

    return solucaoFinalValida;
}



bool resolvedor::verificarSolucao(bool mostrarErros) {
    bool ok = true;

    // 1. Verificar se todos os candidatos estão bem alocados
    for (int i = 0; i < numCandidatos; i++) {
        int e = candidatos[i].escola;
        int s = candidatos[i].sala;
        int t = candidatos[i].tipo;

        if (e == ESCOLA_NENHUMA) {
            if (mostrarErros) {
                std::cerr << "[ERRO] Candidato " << i << " nao alocado!\n";
            }
            ok = false;
            continue;
        }

        if (!locaisProva[e].temTipo(t)) {
            if (mostrarErros) {
                std::cerr << "[ERRO] Candidato " << i << " (tipo " << t << ") na escola " << e << " que nao oferece esse tipo!\n";
            }
            ok = false;
        }

        if (s < 0 || s >= locaisProva[e].numSalas) {
            if (mostrarErros) {
                std::cerr << "[ERRO] Candidato " << i << " na sala invalida " << s << " da escola " << e << "\n";
            }
            ok = false;
        } else if (locaisProva[e].tipoSalas[s] != t) {
            if (mostrarErros) {
                std::cerr << "[ERRO] Candidato " << i << " (tipo " << t << ") na sala " << s << " (tipo " << locaisProva[e].tipoSalas[s] << ")\n";
            }
            ok = false;
        }
    }

    // 2. Verificar as escolas e salas
    for (int j = 0; j < numLocais; j++) {
        LocalProva& lp = locaisProva[j];

        if (lp.numTipos > lp.maxTipos) {
            if (mostrarErros) {
                std::cerr << "[ERRO] Escola " << j << " tem " << lp.numTipos << " tipos (max " << lp.maxTipos << ")\n";
            }
            ok = false;
        }

        int somaSalas = 0;
        int somaAlunos = 0;
        for (int t = 0; t < NUM_TIPOS; t++) somaAlunos += lp.alunosPorTipo[t];

        for (int s = 0; s < lp.numSalas; s++) {
            somaSalas += lp.ocupSalas[s];
            if (lp.ocupSalas[s] > lp.capSalas[s]) {
                if (mostrarErros) {
                    std::cerr << "[ERRO] Escola " << j << " sala " << s << " superlotada: " << lp.ocupSalas[s] << "/" << lp.capSalas[s] << "\n";
                }
                ok = false;
            }
        }

        if (lp.ocupTotal != somaSalas || lp.ocupTotal != somaAlunos) {
            if (mostrarErros) {
                std::cerr << "[ERRO] Escola " << j << " inconsistencia de ocupacao: total=" << lp.ocupTotal << ", salas=" << somaSalas << ", alunos=" << somaAlunos << "\n";
            }
            ok = false;
        }

        // Contar alunos reais na escola j
        int reais[NUM_TIPOS] = {0};
        for (int i = 0; i < numCandidatos; i++) {
            if (candidatos[i].escola == j) reais[candidatos[i].tipo]++;
        }
        for (int t = 0; t < NUM_TIPOS; t++) {
            if (lp.alunosPorTipo[t] != reais[t]) {
                if (mostrarErros) {
                    std::cerr << "[ERRO] Escola " << j << " contador tipo " << t << " difere da alocacao real.\n";
                }
                ok = false;
            }
        }
    }

    return ok;
}

void resolvedor::imprimirResumoSolucao() {
    std::cout << "\n--- RESUMO DAS ESCOLAS ---\n";
    for (int j = 0; j < numLocais; j++) {
        LocalProva& lp = locaisProva[j];
        std::cout << "Escola " << j << " | Tipos: ";
        for (int t = 0; t < lp.numTipos; t++) {
            std::cout << lp.tipos[t] << " ";
        }
        std::cout << "| Ocupacao: " << lp.ocupTotal << "/" << lp.capTotal << "\n";

        std::cout << "  Salas: ";
        for (int s = 0; s < lp.numSalas; s++) {
            std::cout << "[" << lp.ocupSalas[s] << "/" << lp.capSalas[s] << " T" << lp.tipoSalas[s] << "] ";
        }
        std::cout << "\n";
    }
}





/*
bool resolvedor::fase2(){
    std::mt19937 gen(seed);
    std::vector<std::array<Aresta, 3>> top3(numCandidatos);
    double pontuacaoArestaCalculada = 0.0;

    arestaValida.assign(numLocais, std::vector<bool>(numCandidatos, false));
    totalArestas.assign(numCandidatos, 0);
    std::set<std::pair<int, int>> fila;

    for(int i=0; i < numCandidatos; i++){
        top3[i] = {{
            {1e18, -1}, {1e18, -1}, {1e18, -1}
        }};

        montarTop3(top3, i);
    }

    // Ordem aleatória para o caminho não-crítico
    std::vector<int> ordem(numCandidatos);
    for(int i = 0; i < numCandidatos; i++) ordem[i] = i;
    std::shuffle(ordem.begin(), ordem.end(), gen);
    int proximo = 0;

    int restantes = numCandidatos;

    std::uniform_int_distribution<int> distCritico(0, 1);
    int numAleatorio = 0;
    int localAlocar = 0;

    while(restantes != 0){
        int critico = primeiroCritico(top3);

        while(critico != -1 && restantes != 0){
            int t = candidatos[critico].tipo;
            bool unicaPos = (top3[critico][1].pontuacao >= 1e17);

            if(top3[critico][0].pontuacao >= 1e17)
                return false;

            numAleatorio = unicaPos ? 0 : distCritico(gen);

            localAlocar = top3[critico][numAleatorio].escola;
            bool tipoCriado = !locaisProva[localAlocar].temTipo(t);

            int sala = locaisProva[localAlocar].alocarCandidato(t);

            if(sala == -1)
                std::cout<<critico<<std::endl;
            candidatos[critico].escola = localAlocar;
            candidatos[critico].sala = sala;

            if(!verificaViabilidadeAtual()){
                // desfaz
                locaisProva[localAlocar].desalocarCandidato(t, sala);
                if(tipoCriado) locaisProva[localAlocar].retirarTipo(t);
                candidatos[critico].escola = ESCOLA_NENHUMA;
                candidatos[critico].sala = SALA_NENHUMA;

                if(unicaPos) return false;

                numAleatorio = 1 - numAleatorio;
                localAlocar = top3[critico][numAleatorio].escola;
                bool tipoCriado2 = !locaisProva[localAlocar].temTipo(t);

                locaisProva[localAlocar].adicionarTipo(t, 1);
                int sala2 = locaisProva[localAlocar].alocarCandidato(t);
                candidatos[critico].escola = localAlocar;
                candidatos[critico].sala = sala2;

                if(!verificaViabilidadeAtual()){
                    locaisProva[localAlocar].desalocarCandidato(t, sala2);
                    if(tipoCriado2) locaisProva[localAlocar].retirarTipo(t);
                    candidatos[critico].escola = ESCOLA_NENHUMA;
                    candidatos[critico].sala = SALA_NENHUMA;
                    return false;
                }
            }

            candidatosAlocadosTipo[t]++;
            restantes--;

            atualizarTop3Escola(top3, localAlocar);

            critico = primeiroCritico(top3);
        }

        if(restantes == 0) break;

        // Caminho não-crítico
        while(proximo < numCandidatos &&
              candidatos[ordem[proximo]].escola != ESCOLA_NENHUMA)
            proximo++;

        if (proximo >= numCandidatos) {
            std::cerr << "Fila esgotada com " << restantes << " não alocados.\n";
            return false;
        }

        int c = ordem[proximo++];
        int t = candidatos[c].tipo;

        double melhor = top3[c][0].pontuacao;
        std::vector<int> rcl;
        for(int k = 0; k < 3; k++){
            if(top3[c][k].pontuacao < 1e17 &&
               top3[c][k].pontuacao <= melhor * (1.0 + ALPHA))
                rcl.push_back(k);
        }
        if(rcl.empty()){
            std::cerr << "Candidato " << c << " sem escola viavel.\n";
            return false;
        }

        // NOVO: Embaralha a RCL para testar as opções em ordem aleatória
        std::shuffle(rcl.begin(), rcl.end(), gen);
        bool alocou = false;

        for(int k = 0; k < (int)rcl.size(); k++){
            int pos = rcl[k];
            int localAlocar = top3[c][pos].escola;

            bool tipoCriado = !locaisProva[localAlocar].temTipo(t);
            int sala = locaisProva[localAlocar].alocarCandidato(t);

            candidatos[c].escola = localAlocar;
            candidatos[c].sala = sala;

            // VERIFICAÇÃO DE VIABILIDADE AQUI
            if(verificaViabilidadeAtual()){
                candidatosAlocadosTipo[t]++;
                restantes--;
                atualizarTop3Escola(top3, localAlocar);
                alocou = true;
                break; // Alocação segura encontrada
            } else {
                // Se quebrou a viabilidade, desfaz e tenta a próxima da RCL
                locaisProva[localAlocar].desalocarCandidato(t, sala);
                if(tipoCriado) locaisProva[localAlocar].retirarTipo(t);
                candidatos[c].escola = ESCOLA_NENHUMA;
                candidatos[c].sala = SALA_NENHUMA;
            }
        }

        if(!alocou) {
            // Nenhuma opção da RCL manteve a viabilidade global
            std::cerr << "Nenhuma opção da RCL manteve a viabilidade.\n";
            return false;
        }
    }

    return true;
}
*/


/*
void resolvedor::atualizarTop3Escola(
    std::vector<std::array<Aresta, 3>>& top3,
    int j,
    std::set<std::pair<int, int>>& fila)
{
    for(int k = 0; k < numCandidatos; k++){
        if(candidatos[k].escola != ESCOLA_NENHUMA) continue;

        // 1. O candidato k já tem a escola j no top3?
        int pos = -1;
        for(int m = 0; m < 3; m++){
            if(top3[k][m].escola == j){
                pos = m;
                break;
            }
        }

        if (pos != -1) {
            // --- CASO A: A escola j estava no top3 ---

            double novo = pontuacaoAresta(k, j, candidatos[k].tipo);
            bool eraValido = arestaValida[k][j];
            bool ehValido = (novo < 1e17);

            // Atualiza contador/fila se o estado mudou
            if (eraValido != ehValido) {
                fila.erase({totalArestas[k], k});
                if (ehValido) { totalArestas[k]++; arestaValida[k][j] = true; }
                else { totalArestas[k]--; arestaValida[k][j] = false; }
                fila.insert({totalArestas[k], k});
            }

            double pontuacao_antiga = top3[k][pos].pontuacao;

            // Se a escola piorou (a pontuação subiu) OU ficou inválida:
            if (!ehValido || novo > pontuacao_antiga) {

                // 1. Se ficou inválida, marca como vazia
                if (!ehValido) {
                    top3[k][pos].pontuacao = 1e18;
                    top3[k][pos].escola = -1;
                } else {
                    // Se apenas piorou, atualiza a pontuação
                    top3[k][pos].pontuacao = novo;
                }

                // 2. Busca o melhor "de fora" para substituir, se for melhor que a pior do top3
                // Primeiro, encontramos qual é a pior posição atual do top3 (para saber o limiar)
                int pior_pos = 0;
                for(int m = 1; m < 3; m++) {
                    if(top3[k][m].pontuacao > top3[k][pior_pos].pontuacao) pior_pos = m;
                }

                double limiar = top3[k][pior_pos].pontuacao;

                double melhor_p = limiar;
                int melhor_j = -1;

                for(int jj = 0; jj < numLocais; jj++){
                    if(jj == j) continue;
                    if(!arestaValida[k][jj]) continue;

                    bool noTop3 = false;
                    for(int m = 0; m < 3; m++){
                        if(top3[k][m].escola == jj){ noTop3 = true; break; }
                    }
                    if(noTop3) continue;

                    // Se a escola jj for melhor que o pior do top3, ela entra
                    double p = pontuacaoAresta(k, jj, candidatos[k].tipo);
                    if(p < melhor_p){
                        melhor_p = p;
                        melhor_j = jj;
                    }
                }

                if(melhor_j != -1){
                    top3[k][pior_pos] = {melhor_p, melhor_j};
                }
            } else {
                // A escola melhorou (a pontuação caiu), apenas atualiza
                top3[k][pos].pontuacao = novo;
            }
        } else {
            // --- CASO B: A escola j NÃO estava no top3 ---

            double novo = pontuacaoAresta(k, j, candidatos[k].tipo);
            bool eraValido = arestaValida[k][j];
            bool ehValido = (novo < 1e17);

            if (eraValido != ehValido) {
                fila.erase({totalArestas[k], k});
                if (ehValido) { totalArestas[k]++; arestaValida[k][j] = true; }
                else { totalArestas[k]--; arestaValida[k][j] = false; }
                fila.insert({totalArestas[k], k});
            }

            // Se a escola j ficou boa o suficiente para entrar no top3
            if (ehValido && novo < top3[k][2].pontuacao) {
                top3[k][2] = {novo, j};
            }
        }

        // Reordena o top3 (o tamanho é 3, então o custo é ínfimo)
        std::sort(top3[k].begin(), top3[k].end(),
                  [](const Aresta& a, const Aresta& b){
                      return a.pontuacao < b.pontuacao;
                  });
    }
}
*/
