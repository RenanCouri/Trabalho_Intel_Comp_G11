#ifndef ESCOLA_H_INCLUDED
#define ESCOLA_H_INCLUDED

class LocalProva{
public:
    int id;
    int* capSalas;
    int numSalas;
    int* ocupSalas;


    int numTipos;
    char* tipos;
    char* tipoSalas;

    int capTotal;
    int ocupTotal;

    LocalProva(int id, int numSalas, int* capSalas, int numTipos );
};

#endif // ESCOLA_H_INCLUDED
