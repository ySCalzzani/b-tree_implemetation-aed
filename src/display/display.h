#ifndef DISPLAY_H
#define DISPLAY_H

/* ----------------------------------------------------------------------------
 * display.h
 * Visualização interativa da árvore B. Toda a impressão vive aqui — a BTree
 * é só lógica e expõe apenas acessores de leitura (getRoot/getNode).
 *
 * Header-only e templado porque as funções operam sobre BTree<M>&.
 *
 * O modo interativo "congela" a tela: clearScreen() limpa e reposiciona o
 * cursor no topo via sequência ANSI, e o main redesenha menu + árvore a cada
 * passo, pausando entre operações.
 * ------------------------------------------------------------------------- */

#include <iostream>
#include <queue>
#include <utility>

#include "btree.h"
#include "node.h"

// ANSI: \033[2J limpa a tela inteira, \033[H move o cursor para o canto
// superior esquerdo. Juntos dão o efeito de "congelar" e redesenhar.
inline void clearScreen() {
    std::cout << "\033[2J\033[H";
}

// Imprime as chaves de um nó no formato: [k1 k2 ...] (rec N)
template <int M>
void printKeys(const BTreeNode<M>& p, int record) {
    std::cout << "[";
    for (int k = 1; k <= p.numKeys; k++) {
        if (k > 1) std::cout << " ";
        std::cout << p.K[k];
    }
    std::cout << "] (rec " << record << ")";
}

template <int M>
static void displayHelper(BTree<M>& tree, int record, int depth) {
    if (record == 0) return;
    BTreeNode<M> p = tree.getNode(record);

    for (int i = 0; i < depth; i++) std::cout << "  ";
    printKeys(p, record);
    std::cout << std::endl;

    // Recorre por todos os filhos A[0..numKeys].
    for (int k = 0; k <= p.numKeys; k++) {
        displayHelper(tree, p.A[k], depth + 1);
    }
}

// View indentada (hierárquica): cada filho aparece sob o pai, com indentação
// extra. Boa para entender as relações pai/filho.
template <int M>
void displayTree(BTree<M>& tree) {
    int root = tree.getRoot();
    if (root == 0) {
        std::cout << "(arvore vazia)" << std::endl;
        return;
    }
    displayHelper(tree, root, 0);
}

// View por nível (BFS): todos os nós de uma mesma profundidade numa linha.
// Boa para ler o formato da árvore por nível.
template <int M>
void displayLevelOrder(BTree<M>& tree) {
    int root = tree.getRoot();
    if (root == 0) {
        std::cout << "(arvore vazia)" << std::endl;
        return;
    }

    std::queue<std::pair<int, int>> q;
    q.push({root, 0});
    int currentDepth = -1;

    while (!q.empty()) {
        auto [record, depth] = q.front();
        q.pop();

        if (depth != currentDepth) {
            if (currentDepth >= 0) std::cout << std::endl;
            std::cout << "Nivel " << depth << ": ";
            currentDepth = depth;
        }

        BTreeNode<M> p = tree.getNode(record);
        printKeys(p, record);
        std::cout << "  ";

        for (int k = 0; k <= p.numKeys; k++) {
            if (p.A[k] != 0) q.push({p.A[k], depth + 1});
        }
    }
    std::cout << std::endl;
}

// Dump bruto do arquivo: imprime TODOS os registros do .dat em texto legível,
// um por linha, na ordem física em que estão gravados. Diferente das views
// acima (que só percorrem nós alcançáveis a partir da raiz), aqui mostramos o
// arquivo inteiro — incluindo o registro 0 (cabeçalho, cujo A[0] guarda o id da
// raiz) e eventuais nós antigos/livres. Útil para "ver" o binário .dat como
// texto, já que o arquivo é uma sequência de structs BTreeNode<M> em bytes crus.
//
// O número de registros vem do tamanho do arquivo / sizeof(BTreeNode<M>).
template <int M>
void dumpFile(BTree<M>& tree) {
    long bytes = tree.getDiskManager().getFileSize();
    int numRecords = (int)(bytes / sizeof(BTreeNode<M>));

    std::cout << "--- Dump bruto do .dat (" << numRecords << " registros, "
              << bytes << " bytes, sizeof(no)=" << sizeof(BTreeNode<M>)
              << ") ---\n";
    std::cout << "raiz: rec " << tree.getRoot() << "\n";

    for (int id = 0; id < numRecords; id++) {
        BTreeNode<M> p = tree.getNode(id);

        std::cout << "rec " << id;
        if (id == 0) std::cout << " (cabecalho)";
        std::cout << ": numKeys=" << p.numKeys;

        // K[1..numKeys] entre os ponteiros A[0..numKeys]: A0 K1 A1 K2 A2 ...
        std::cout << " | A[0]=" << p.A[0];
        for (int k = 1; k <= p.numKeys; k++) {
            std::cout << " K[" << k << "]=" << p.K[k]
                      << " A[" << k << "]=" << p.A[k];
        }
        std::cout << "\n";
    }
}

// Percorre a árvore procurando 'key' e imprime se foi encontrada e em qual
// nó/índice. A lógica de traversal para exibição vive aqui (não na BTree).
template <int M>
void displaySearchResult(BTree<M>& tree, int key) {
    int currentID = tree.getRoot();

    while (currentID != 0) {
        BTreeNode<M> node = tree.getNode(currentID);
        int i = node.findIndex(key);

        if (i > 0 && node.K[i] == key) {
            std::cout << "Chave " << key << " ENCONTRADA"
                      << " (rec " << currentID << ", K[" << i << "])" << std::endl;
            return;
        }

        // Nó folha: A[0] == 0 marca ausência de filhos.
        if (node.A[0] == 0) break;

        currentID = node.A[i];
    }

    std::cout << "Chave " << key << " NAO encontrada." << std::endl;
}

#endif
