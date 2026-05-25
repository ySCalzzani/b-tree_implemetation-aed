#ifndef NODE_H
#define NODE_H

#include <fstream>
#include <iostream>

/* ----------------------------------------------------------------------------
* BTreeNode: Representa um nó de uma árvore B de ordem M.
* - numKeys: número de chaves atualmente armazenadas no nó.
* - K: array de chaves.
* - A: array de ponteiros para filhos.
* Construtor: Inicializa um nó vazio com numKeys = 0 e chaves/ponteiros zerados.
* findIndex: Retorna o índice onde a chave foi encontrada ou onde deveria ser inserida.
* ------------------------------------------------------------------------- */
template <int M>
struct BTreeNode {
public:
    int numKeys;
    int K[M+1];
    int A[M+1];

    BTreeNode() {
        numKeys = 0;
        for (int i = 0; i <= M; i++) {
            K[i] = 0;
            A[i] = 0;
        }
    }

    int findIndex(int key) const {
        int i = 0;
        while (i < numKeys && key >= K[i + 1]) {
            i++;
        }
        return i;
    }
};

#endif