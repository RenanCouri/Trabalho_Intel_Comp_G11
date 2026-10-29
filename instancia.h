#ifndef INSTANCIA_H_INCLUDED
#define INSTANCIA_H_INCLUDED


#include <vector>
#include <string>

struct DadosEscola {
    int numSalas;
    std::vector<int> caps;
    float lat, lon;
};

struct Instancia {
    int numCandidatos = 0;
    int numLocais     = 0;

    std::vector<int> tipos;            // [numCandidatos]
    std::vector<DadosEscola> escolas;  // [numLocais]
    float** matDist = nullptr;         // [numCandidatos][numLocais]
};

// Retorna nullptr em caso de erro (arquivo inexistente, formato inválido).
Instancia* lerInstancia(const std::string& caminho);

// Libera matDist e a própria struct. Não libera tipos/escolas (são std::vector).
void liberarInstancia(Instancia* inst);


#endif // INSTANCIA_H_INCLUDED
