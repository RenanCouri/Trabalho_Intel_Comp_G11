#ifndef LOCAL_PROVA_H_INCLUDED
#define LOCAL_PROVA_H_INCLUDED

#include <random>

#include "tipo_prova.h"

class LocalProva {
public:
    int id;
    int* capSalas;
    int numSalas;
    int* ocupSalas;

    int seed = 42;
    std::mt19937 gen;
    std::uniform_int_distribution<int> dist;

    int maxTipos;
    int numTipos;
    int tiposAlocados;
    int* tipos;
    int* tipoSalas;

    int capTotal;
    int ocupTotal;


    float lat, lon;


    int* alunosPorTipo;   // tamanho NUM_TIPOS

    LocalProva();
    LocalProva(int id, int numSalas, int* capSalas, int maxTipos);
    ~LocalProva();

    // consultas e modificações usadas pelas fases 1 e 2
    bool podeReceber(int t);
    int  vagasLivresTipo(int t);
    int vagasLivresPotenciaisTipo(int t);

    bool adicionarTipo(int t, int nSalas);
    int alocarNovasSalas(int t, int nSalas);
    int alocarCandidato(int t);
    void desalocarCandidato(int t, int sala);
    void retirarTipo(int t);
    bool temTipo(int t);
    int posTipo(int t);
};



#endif // LOCAL_PROVA_H_INCLUDED
