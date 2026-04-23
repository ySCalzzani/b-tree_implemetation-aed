#include <iostream>
#include <fstream>
#include <limits>

using namespace std;

const int MAIS_INFINITO = numeric_limits<int>::max();
const int MENOS_INFINITO = numeric_limits<int>::min();
const int m = 3;

struct no {
    int n;
    int K[m+1];
    int A[m+1];
};

struct resultadoBusca {
    int p;
    int i;
    bool encontrado;
};

void escreva(fstream &f, int registro, no p){
    f.seekp(registro * sizeof(no));
    f.write((const char *) (&p), (sizeof(no)));
};

no leia(fstream &f, int registro){
    no p;
    f.seekg(registro * sizeof(no));
    f.read((char*) (&p), sizeof(no));
    return p;
};

int encontre(int x, no p) {
    int i = 0;
    while (i < p.n && x >= p.K[i + 1]) {
        i++;
    }
    return i;
}

resultadoBusca mSearch(fstream &f, int T, int x){
    int p = T; // raiz
    int q = 0;
    int i;

    while (p != 0) {
        no noAtual = leia(f, p);
        
        noAtual.K[0] = MENOS_INFINITO;
        noAtual.K[noAtual.n + 1] = MAIS_INFINITO;

        i = encontre(x, noAtual);

        if (x == noAtual.K[i]){
            return resultadoBusca{p, i, true};
        };

        q = p;
        p = noAtual.A[i];
    };

    return resultadoBusca{q, i, false};
};

int main(){
    no p;

    fstream arvoreB("tmp/arvoreB.dat", ios::in | ios::out | ios::trunc | ios::binary);

    // escreve registro 0 (c/ lixo)
    escreva(arvoreB, 0, p);

    // registro 1
    p.n = 2; p.A[0] = 2; p.K[1] = 20; p.A[1] = 3; p.K[2] = 40; p.A[2] = 4;
    escreva(arvoreB, 1, p);

    // registro 2
    p.n = 2; p.A[0] = 0; p.K[1] = 10; p.A[1] = 0; p.K[2] = 15; p.A[2] = 0;
    escreva(arvoreB, 2, p);

    // registro 3
    p.n = 2; p.A[0] = 0; p.K[1] = 25; p.A[1] = 0; p.K[2] = 30; p.A[2] = 5;
    escreva(arvoreB, 3, p);

    // registro 4
    p.n = 2; p.A[0] = 0; p.K[1] = 45; p.A[1] = 0; p.K[2] = 50; p.A[2] = 0;
    escreva(arvoreB, 4, p);

    // registro 5
    p.n = 1; p.A[0] = 0; p.K[1] = 35; p.A[1] = 0;
    escreva(arvoreB, 5, p);

    // testar a busca
    int input;

    while (true){
        cout << "\nDigite o número para buscar: ";
        cin >> input;

        resultadoBusca resultado = mSearch(arvoreB, 1, input); // raiz 1

        cout << "Valor encontrado: " << (resultado.encontrado ? "Sim" : "Não") << endl;
        cout << "Nó " << resultado.p << " - K[" << resultado.i  << "]" << endl;
    };
    
    arvoreB.close();

    return 0;
}