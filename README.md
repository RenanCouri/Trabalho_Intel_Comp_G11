## Compilação e execução

### Requisitos

- Compilador C++ com suporte ao padrão C++17 (como o g++).
- Arquivos-fonte do projeto (`.cpp` e `.h`).
- Pasta `instancias`, contendo os arquivos de entrada.

### Compilação

A partir do diretório raiz do projeto, execute:

```bash
g++ -std=c++17 -O2 *.cpp -o programa.exe
```

### Execução
Ainda no diretório raiz, execute:

No Windows (PowerShell):
`.\programa.exe`


No terminal MSYS2 UCRT64:
`./programa.exe`

Durante a execução, o usuário deve informar duas opções pelo
teclado. A primeira corresponde ao número máximo de tipos de
prova que cada escola poderá oferecer, podendo ser escolhido
um ou dois tipos. Essa configuração é utilizada durante toda
a execução do algoritmo, tanto na construção da solução quanto
na busca local. Independentemente do limite selecionado,
cada sala pode receber candidatos de apenas um tipo de prova.

A segunda opção corresponde à instância a ser executada.
Estão disponíveis 28 arquivos de entrada, identificados de
Instancia1\_G1.txt a Instancia2\_G14.txt.
O usuário pode selecionar uma instância individualmente ou
escolher a opção de executar todas as instâncias em sequência.

Para cada instância selecionada, são realizadas dez execuções,
utilizando diferentes sementes aleatórias. Ao término, o programa
apresenta as estatísticas das soluções encontradas, incluindo
a quantidade de execuções viáveis, as distâncias inicial e final,
a melhoria obtida e o tempo de execução. Quando apenas uma
instância é selecionada, também são apresentados os detalhes
da melhor solução viável encontrada.
