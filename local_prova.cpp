#include "local_prova.h"

#include <iostream>
#include <cstdlib>


LocalProva::LocalProva()
{
    id = -1;
    capSalas = nullptr;
    numSalas = 0;
    ocupSalas = nullptr;

    numTipos = 0;
    maxTipos = 0;
    tipos = nullptr;
    tipoSalas = nullptr;

    capTotal = 0;
    ocupTotal = 0;

    lat = lon = 0.0f;

    alunosPorTipo = nullptr;
}


LocalProva::LocalProva(int id, int numSalas, int* capSalas, int maxTipos )
{
    this->id = id;
    if(id < 0)
    {
        std::cerr << "id invalido" << std::endl;
        std::exit(1);
    }

    this->numSalas = numSalas;
    this->capSalas = capSalas;
    this->maxTipos = maxTipos;

    capTotal = ocupTotal = 0;
    numTipos = 0;

    ocupSalas = new int[numSalas];
    tipoSalas = new int[numSalas];
    for(int i=0; i < numSalas; i++)
    {
        ocupSalas[i] = 0;
        tipoSalas[i] = -1;
        capTotal += capSalas[i];
    }

    tipos = new int[maxTipos];
    for(int i=0; i<maxTipos; i++)
    {
        tipos[i] = -1;
    }

    alunosPorTipo = new int[NUM_TIPOS];
    for(int t = 0; t < NUM_TIPOS; t++)
        alunosPorTipo[t] = 0;

    dist = std::uniform_int_distribution<int>(0, numSalas-1);

    gen.seed(42 + id);







}


LocalProva::~LocalProva()
{
    delete [] capSalas;
    delete [] ocupSalas;
    delete [] tipos;
    delete [] tipoSalas;
    delete [] alunosPorTipo;
}


bool LocalProva::podeReceber(int t)
{

    int tipoPresente = -1;
    for(int i=0; i < numTipos; i++)
        if(tipos[i] == t)
        {
            tipoPresente = t;
            break;
        }

    if(tipoPresente == -1 && numTipos == maxTipos)
        return false;

    for(int i=0; i<numSalas; i++)
    {
        if(tipoSalas[i] == -1)
            return true;
        else if(tipoSalas[i] == tipoPresente && capSalas[i] - ocupSalas[i] > 0)
            return true;

    }


    return false;


}

int LocalProva::vagasLivresTipo(int t)
{
    bool temTipo = false;
    for(int i=0; i < numTipos; i++)
        if(tipos[i] == t)
            temTipo = true;
    if(!temTipo)
        return 0;

    int contador = 0;
    for(int i=0; i<numSalas; i++)
    {
        if(tipoSalas[i] == t)
            contador += capSalas[i] - ocupSalas[i];
    }

    return contador;

}


int LocalProva::vagasLivresPotenciaisTipo(int t)
{
    bool temTipo = false;
    for(int i=0; i < numTipos; i++)
        if(tipos[i] == t)
            temTipo = true;
    if(!temTipo)
        return 0;

    int contador = 0;
    for(int i=0; i<numSalas; i++)
    {
        if(tipoSalas[i] == -1)
            contador+=capSalas[i];
        else if(tipoSalas[i] == t)
            contador += capSalas[i] - ocupSalas[i];
    }

    return contador;

}


int LocalProva::alocarNovasSalas(int t, int nSalas)
{
    int salasAtrb = 0;

    int salaBase = dist(gen);

    int ultimaSala = -1;
    for(int i = salaBase; i - salaBase < numSalas; i++)
    {
        int idx = (i%numSalas);
        if(tipoSalas[idx] == -1)
        {
            tipoSalas[idx] = t;
            salasAtrb ++;
            ultimaSala = idx;
            if(salasAtrb == nSalas)
                break;
            salaBase = dist(gen);
            i = salaBase;
        }

    }

    return ultimaSala;

}


bool LocalProva::adicionarTipo(int t, int nSalas)
{

    for(int i=0; i<numTipos; i++)
        if(tipos[i] == t)
            return true;


    if(numTipos == maxTipos)
        return false;


    if(nSalas == 0)
        nSalas = 1;

    int ultimaSala = alocarNovasSalas(t, nSalas);
    if(ultimaSala == -1)
        return false;

    numTipos++;

    tipos[numTipos - 1] = t;

    return true;

}


int LocalProva::alocarCandidato(int t)
{

    if(!adicionarTipo(t, 1))
        return -1;

    int sala = -1;

    for(int i=0; i<numSalas; i++)
    {
        if(tipoSalas[i] == t && capSalas[i] - ocupSalas[i] > 0)
        {
            sala = i;
            ocupSalas[i]++;
            alunosPorTipo[t]++;
            ocupTotal++;
            break;
        }
    }

    if(sala != -1)
        return sala;


    int ns = alocarNovasSalas(t, 1);
    if(ns != -1)
    {
        ocupSalas[ns]++;
        alunosPorTipo[t]++;
        ocupTotal++;

    }

    return ns;


}

bool LocalProva::temTipo(int t)
{

    for(int i=0; i<numTipos; i++)
        if(tipos[i] == t)
            return true;

    return false;
}


int LocalProva::posTipo(int t)
{

    for(int i=0; i<numTipos; i++)
        if(tipos[i] == t)
            return i;

    return -1;
}

void LocalProva::retirarTipo(int t)
{
    int pos = posTipo(t);
    if(pos != -1)
    {
        for(int i=0; i<numSalas; i++)
        {
            if(tipoSalas[i] == t)
            {
                tipoSalas[i] = -1;
                ocupSalas[i] = 0;
            }
        }
        numTipos --;
        tipos[pos] = -1;
        ocupTotal -= alunosPorTipo[t];
        alunosPorTipo[t] = 0;
        for(int j = pos; j < numTipos; j++)
            tipos[j] = tipos[j+1];

    }


}


void LocalProva::desalocarCandidato(int t, int sala)
{
    if(sala < 0 || sala >= numSalas) return;
    if(ocupSalas[sala] > 0) ocupSalas[sala]--;
    if(alunosPorTipo[t] > 0) alunosPorTipo[t]--;
    ocupTotal--;
}
