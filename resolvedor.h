#ifndef RESOLVEDOR_H_INCLUDED
#define RESOLVEDOR_H_INCLUDED

#include "local_prova.h"
#include "tipo_prova.h"
#include <vector>
#include <array>
#include <set>



typedef struct {
    int tipo;
    int escola = -1;
    int sala   = -1;
} Candidato;


typedef struct {
    double pontuacao;
    int escola;
} Aresta;




typedef struct {
        double distancia;
        int id_cand;
} Aluno ;


typedef struct {
    double valor;      // d[c][j_atual] - d[c][i_destino]  (positivo = ganho real)
    int id_cand;       // id do candidato topo (ou -1 se vazio)
} Ganho;



class resolvedor {
public:
    int numCandidatos;
    int numLocais;
    float** matDist;
    LocalProva* locaisProva;
    Candidato* candidatos;
    int maxTipos;

    Ganho*** melhorGanho;

    //unsigned int seed = 42;
    unsigned int seed = 442;
    //unsigned int seed = 4458;
    int n_atribuicoes = 0;

    int    SALAS_FASE1       = 4;
    int    MAX_ESCOLAS_FASE1 = 2;
    int    MAX_ESCOLAS_FASE1_TIPO = 2;
    int MAX_TIPOS_ESCOLA = 2;
    double ALPHA             = 0.15;
    double BIAS_REL          = 1.00;
    double TETO_AGLOM        = 2.00;
    double BASE_DISP         = 0.50;

    resolvedor(float** matDist, int numCandidatos, int numLocais, int maxTipos);
    ~resolvedor();

    void resetarEstado();

    void construir();
    bool fase1Fallback();
    void fase1(bool forcarAncora);
    bool fase2();

    bool particaoObviamenteInviavel();

    double indicePreferencia(int j, int t);
    double disponibilidade(int j, int t);
    double aglomeracao(int j, int t);
    double pontuacaoAresta(int c, int j, int t);

    void calculoPontuacao(
        const std::vector<std::vector<double>>& indPref,
        std::vector<std::vector<double>>& pontuacoes);
    void calcularIndPref(std::vector<std::vector<double>>& indPref);

    float avaliar() const;
    void  salvarSolucao(const char* caminho);

    bool verificaViabilidadeAtual();

    int primeiroCritico(const std::vector<std::array<Aresta, 3>> &top3);

    void montarTop3(std::vector<std::array<Aresta, 3>> &top3, int i);

    void atualizarTop3Escola(std::vector<std::array<Aresta, 3>>& top3, int j);

    void atualizarArestas(int j,int tipoAlocado,bool atualizarTodos, std::set<std::pair<int, int>>& fila);

    float avaliar();

    bool verificarSolucao(bool mostrarErros = true);
    void imprimirResumoSolucao();



    bool buscaLocal(int K);



private:
    int escolasDoTipo[NUM_TIPOS];
    std::vector<std::vector<double>> mediaDist;
    std::vector<double> fracao_candidatos_tipo;
    int candidatosAlocadosTipo[NUM_TIPOS];
    int candidatosPorTipo[NUM_TIPOS];

    std::vector<std::vector<bool>> arestaValida; // Matriz de estado
    std::vector<int> totalArestas;               // Contador dinâmico

    int* melhoresCands;
    Ganho** ganhoTipos;
    int* contadores;

    double* trocasTipo;   // tamanho: numLocais * numLocais * MAX_TIPOS * MAX_TIPOS

    double* sumDist;   // tamanho: numLocais * NUM_TIPOS * numLocais

    inline int idxSum(int X, int t, int Y) const {
        return (X * NUM_TIPOS + t) * numLocais + Y;
    }

    inline int idxTroca(int i, int j, int a, int b) const {
        return ((i * numLocais + j) * maxTipos + a) * maxTipos + b;
    }
    bool buscaLocal_MoverCandidato(int K);

    void construirTabelaGanhos();
    void atualizarTabelaGanhos(int X, int Y, int t, int A, int B);
    bool buscaLocal_TrocarCandidatos();

    bool buscaLocal_AlocarTipo();

    void construirSumDist();
    void construirTrocasTipo();
    bool buscaLocal_MudarTipo_1_1();

    bool buscaLocal_TransferirSalas();




};


#endif // RESOLVEDOR_H_INCLUDED
