#include "instancia.h"
#include "tipo_prova.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdlib>

static std::string trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

static std::vector<std::string> split(const std::string& s, char sep) {
    std::vector<std::string> res;
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, sep))
        res.push_back(trim(item));
    return res;
}

static int tipoDeString(const std::string& s) {
    if (s == "M1")  return TIPO_M1;
    if (s == "M2")  return TIPO_M2;
    if (s == "M3e") return TIPO_M3e;
    if (s == "M3s") return TIPO_M3s;
    if (s == "M3h") return TIPO_M3h;
    if (s == "M3d") return TIPO_M3d;
    std::cerr << "Tipo desconhecido: " << s << "\n";
    std::exit(1);
}


Instancia* lerInstancia(const std::string& caminho) {
    std::ifstream arq(caminho);
    if (!arq.is_open()) {
        std::cerr << "Nao foi possivel abrir " << caminho << "\n";
        return nullptr;
    }

    Instancia* inst = new Instancia();
    std::vector<std::vector<float>> matTmp;
    std::string linha;
    int secao = 0;

    while (std::getline(arq, linha)) {
        std::string s = trim(linha);
        if (s.empty()) continue;

        if (s[0] == '#') {
            if      (s.find("CANDIDATOS")        != std::string::npos) secao = 1;
            else if (s.find("ESCOLAS")           != std::string::npos) secao = 2;
            else if (s.find("MATRIZ_DISTANCIAS") != std::string::npos) secao = 3;
            continue;
        }

        if (s.find("NUM_CANDIDATOS:") == 0) {
            // Pega tudo após o dois-pontos e converte
            inst->numCandidatos = std::stoi(trim(s.substr(s.find(':') + 1)));
            continue;
        }
        if (s.find("NUM_ESCOLAS:") == 0) {
            inst->numLocais = std::stoi(trim(s.substr(s.find(':') + 1)));
            continue;
        }

        if (secao == 1) {
            auto p = split(s, ',');
            if (p.size() < 5) continue;
            inst->tipos.push_back(tipoDeString(p[2]));
        }
        else if (secao == 2) {
            auto p = split(s, ',');
            if (p.size() < 6) continue;
            DadosEscola e;
            e.lat = std::stof(p[2]);
            e.lon = std::stof(p[3]);
            e.numSalas = std::stoi(p[4]);
            auto caps = split(p[5], ';');
            for (auto& c : caps) e.caps.push_back(std::stoi(c));
            if ((int)e.caps.size() != e.numSalas) {
                std::cerr << "Escola com num_salas inconsistente\n";
                delete inst;
                return nullptr;
            }
            inst->escolas.push_back(e);
        }
        else if (secao == 3) {
            std::string ss = s;
            for (auto& ch : ss) if (ch == ',') ch = ' ';
            std::stringstream ss2(ss);
            std::vector<float> row;
            float v;
            while (ss2 >> v) row.push_back(v);
            matTmp.push_back(row);
        }
    }
    arq.close();

    // --- Validações ---
    if ((int)inst->tipos.size() != inst->numCandidatos) {
        std::cerr << "Candidatos: esperado " << inst->numCandidatos
                  << ", lido " << inst->tipos.size() << "\n";
        delete inst; return nullptr;
    }
    if ((int)inst->escolas.size() != inst->numLocais) {
        std::cerr << "Escolas: esperado " << inst->numLocais
                  << ", lido " << inst->escolas.size() << "\n";
        delete inst; return nullptr;
    }
    if ((int)matTmp.size() != inst->numCandidatos) {
        std::cerr << "Matriz com " << matTmp.size()
                  << " linhas, esperado " << inst->numCandidatos << "\n";
        delete inst; return nullptr;
    }

    // --- Aloca matDist ---
    inst->matDist = new float*[inst->numCandidatos];
    for (int i = 0; i < inst->numCandidatos; i++) {
        if ((int)matTmp[i].size() != inst->numLocais) {
            std::cerr << "Linha " << i << " com " << matTmp[i].size()
                      << " colunas, esperado " << inst->numLocais << "\n";
            liberarInstancia(inst); return nullptr;
        }
        inst->matDist[i] = new float[inst->numLocais];
        for (int j = 0; j < inst->numLocais; j++)
            inst->matDist[i][j] = matTmp[i][j];
    }

    return inst;
}


void liberarInstancia(Instancia* inst) {
    if (!inst) return;
    if (inst->matDist) {
        for (int i = 0; i < inst->numCandidatos; i++)
            delete[] inst->matDist[i];
        delete[] inst->matDist;
    }
    delete inst;
}
