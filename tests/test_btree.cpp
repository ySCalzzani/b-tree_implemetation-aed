/* ----------------------------------------------------------------------------
 * test_btree.cpp
 * Testes com dados de mock para BTree<M>, cobrindo search(), insert() e remove().
 *
 * Cada caso descreve a árvore "antes" e a árvore "final" esperada como um
 * fixture de texto legível (tests/fixtures/*.txt) carregado por
 * BTree::loadFromFile(). A operação roda sobre a árvore-antes e o resultado é
 * comparado com a árvore final.
 *
 * A comparação é LÓGICA: percorremos as duas árvores a partir da raiz seguindo
 * os ponteiros de filhos, comparando a quantidade de chaves de cada nó, as
 * chaves e o formato da subárvore. Os ids físicos dos registros e os nós
 * abandonados no disco são ignorados — os slides de remoção deixam, de
 * propósito, nós mortos para trás, então uma comparação byte a byte/por
 * registro seria mal definida.
 *
 * btree.h é header-only, então este arquivo tem seu próprio main() e é compilado
 * de forma independente (nunca linka src/main.o). Rode com `make test`.
 *
 * search(), insert()/split() e remove() estão todos implementados, então todo
 * caso aqui deve PASSAR. A operação ainda roda em um processo filho (fork) para
 * que uma regressão que cause crash (ex.: um allocateNode com bug) apareça como
 * um único FAIL reportado em vez de abortar a suíte inteira.
 * ------------------------------------------------------------------------- */
#include <iostream>
#include <fstream>
#include <string>
#include <cstdio>
#include <sys/wait.h>
#include <unistd.h>

#include "btree.h"

static const std::string FIX = "tests/fixtures/";
static const std::string TMP = "tmp/";

static int g_pass = 0;
static int g_fail = 0;

static void check(const std::string& name, bool cond, const std::string& detail = "") {
    if (cond) {
        std::cout << "  [PASS] " << name << "\n";
        g_pass++;
    } else {
        std::cout << "  [FAIL] " << name;
        if (!detail.empty()) std::cout << "  -> " << detail;
        std::cout << "\n";
        g_fail++;
    }
}

template <int M>
static void readRecord(std::ifstream& f, int id, BTreeNode<M>& node) {
    f.clear();
    f.seekg((long)id * (long)sizeof(BTreeNode<M>));
    f.read((char*)&node, sizeof(BTreeNode<M>));
}

/* Compara recursivamente as subárvores enraizadas nos registros idA / idB. Um
 * id de filho igual a 0 significa "sem filho"; os dois lados precisam concordar
 * nisso. */
template <int M>
static bool compareSubtree(std::ifstream& fa, int idA,
                           std::ifstream& fb, int idB,
                           std::string& msg) {
    if (idA == 0 && idB == 0) return true;
    if (idA == 0 || idB == 0) {
        msg = "formato da arvore difere (um filho e nulo, o outro nao)";
        return false;
    }

    BTreeNode<M> na;
    BTreeNode<M> nb;
    readRecord(fa, idA, na);
    readRecord(fb, idB, nb);

    if (na.numKeys < 0 || na.numKeys > M) {
        msg = "numKeys invalido " + std::to_string(na.numKeys) + " na arvore resultado";
        return false;
    }
    if (na.numKeys != nb.numKeys) {
        msg = "numKeys difere: resultado tem " + std::to_string(na.numKeys) +
              ", esperado " + std::to_string(nb.numKeys);
        return false;
    }
    for (int k = 1; k <= na.numKeys; k++) {
        if (na.K[k] != nb.K[k]) {
            msg = "chave difere: resultado tem " + std::to_string(na.K[k]) +
                  ", esperado " + std::to_string(nb.K[k]);
            return false;
        }
    }
    for (int i = 0; i <= na.numKeys; i++) {
        if (!compareSubtree<M>(fa, na.A[i], fb, nb.A[i], msg)) return false;
    }
    return true;
}

/* Comparação lógica de dois arquivos de dados BTree<M>: mesmas chaves, mesmo
 * formato, partindo da raiz de cada arquivo (registro 0 = cabeçalho, A[0] = id
 * da raiz). */
template <int M>
static bool compareLogical(const std::string& a, const std::string& b, std::string& msg) {
    std::ifstream fa(a, std::ios::binary);
    std::ifstream fb(b, std::ios::binary);
    if (!fa.is_open() || !fb.is_open()) {
        msg = "nao foi possivel abrir o(s) arquivo(s) de dados";
        return false;
    }
    BTreeNode<M> ha;
    BTreeNode<M> hb;
    readRecord(fa, 0, ha);
    readRecord(fb, 0, hb);
    return compareSubtree<M>(fa, ha.A[0], fb, hb.A[0], msg);
}

/* Carrega um fixture em um arquivo .dat novo e o fecha (flush no destrutor). */
template <int M>
static void materialize(const std::string& dat, const std::string& fixture) {
    std::remove(dat.c_str());
    BTree<M> t(dat);
    t.loadFromFile(FIX + fixture);
}

/* fixture antes -> roda op(tree) -> compara com o fixture esperado.
 *
 * O carregamento+op roda em um processo filho (fork): insert() depende de
 * allocateNode()/split(), e um bug nessas funções pode causar crash (ex.:
 * allocateNode passando do fim -> SIGILL). O fork transforma esse crash em um
 * único FAIL reportado em vez de abortar a suíte inteira. */
template <int M, typename Op>
static void runCase(const std::string& name,
                    const std::string& before,
                    Op op,
                    const std::string& after) {
    const std::string actual = TMP + "test_actual.dat";
    const std::string expected = TMP + "test_expected.dat";

    std::remove(actual.c_str());
    std::cout.flush();  // evita que o filho herde saida nao descarregada (flush)

    pid_t pid = fork();
    if (pid == 0) {
        {
            BTree<M> t(actual);
            t.loadFromFile(FIX + before);
            op(t);
        }  // o destrutor faz flush/fecha antes do pai ler o arquivo
        _exit(0);
    }

    int status = 0;
    waitpid(pid, &status, 0);
    if (WIFSIGNALED(status)) {
        check(name, false, "a operacao causou crash (sinal " +
                           std::to_string(WTERMSIG(status)) +
                           ")");
        return;
    }

    materialize<M>(expected, after);

    std::string msg;
    bool ok = compareLogical<M>(actual, expected, msg);
    check(name, ok, msg);
}

/* Wrappers de conveniência para que cada caso caiba em uma linha. */
template <int M>
static void insertCase(const std::string& name, const std::string& before,
                       int key, const std::string& after) {
    runCase<M>(name, before, [key](BTree<M>& t) { t.insert(key); }, after);
}

template <int M>
static void removeCase(const std::string& name, const std::string& before,
                       int key, const std::string& after) {
    runCase<M>(name, before, [key](BTree<M>& t) { t.remove(key); }, after);
}

static void runSearchTests() {
    std::cout << "search() — arvore B do slide 60:\n";
    const std::string dat = TMP + "test_search.dat";
    std::remove(dat.c_str());

    BTree<3> t(dat);
    t.loadFromFile(FIX + "search_tree.txt");

    // Chaves presentes em todos os niveis (arvore dos slides da aula).
    check("A: acha 30 (raiz, slides)",          t.search(30));
    check("B: acha 20 (interno b, slides)",     t.search(20));
    check("C: acha 40 (interno c, slides)",     t.search(40));
    check("D: acha 10 (folha d, slides)",       t.search(10));
    check("E: acha 25 (folha e, slides)",       t.search(25));
    check("F: acha 35 (folha f, slides)",       t.search(35));
    check("G: acha 50 (folha g, slides)",       t.search(50));

    // Chaves ausentes: abaixo, acima e entre as existentes.
    check("H: erra 5 (abaixo de todas, slides)", !t.search(5));
    check("I: erra 33 (lacuna, slides)",         !t.search(33));
    check("J: erra 99 (acima de todas, slides)", !t.search(99));

    // Arvore vazia: toda busca deve errar sem dar crash.
    const std::string empty = TMP + "test_search_empty.dat";
    std::remove(empty.c_str());
    BTree<3> e(empty);            // arquivo novo => rootID == 0
    check("K: erra em arvore vazia",             !e.search(42));
}

static void runInsertTests() {
    std::cout << "insert():\n";
    insertCase<3>("A: insercao sem split", "insert_A_before.txt", 20, "insert_A_after.txt");
    insertCase<3>("B: chave duplicada (no-op)", "insert_B_before.txt", 10, "insert_B_after.txt");
    insertCase<3>("C: insere em arvore vazia", "insert_C_before.txt", 50, "insert_C_after.txt");
    insertCase<3>("D: split da raiz-folha", "insert_D_before.txt", 30, "insert_D_after.txt");

    // Dos slides da aula: insere X=55 (split da folha, mediana promovida).
    insertCase<3>("E: insere 55 (split + promocao, slides)", "insert_E_before.txt", 55, "insert_E_after.txt");

    // Split em cascata: o split da folha estoura o pai, que tambem faz split e
    // promove para uma raiz nova em folha (caminho recursivo do split()).
    insertCase<3>("F: split em cascata (nova raiz)", "insert_F_before.txt", 28, "insert_F_after.txt");

    // Generalidade da ordem: mesma logica de split com m=5 (mid = (M+1)/2 = 3).
    insertCase<5>("G: split da raiz-folha com m=5", "insert_G_before.txt", 25, "insert_G_after.txt");
}

static void runRemoveTests() {
    std::cout << "remove() — sequencia dos slides (dos slides da aula):\n";
    removeCase<3>("A: remove 58 (folha simples, slides)",        "remove_A_before.txt", 58, "remove_A_after.txt");
    removeCase<3>("B: remove 65 (empresta do irmao direito, slides)",   "remove_B_before.txt", 65, "remove_B_after.txt");
    removeCase<3>("C: remove 55 (merge com irmao, slides)",  "remove_C_before.txt", 55, "remove_C_after.txt");
    removeCase<3>("D: remove 40 (merge em cascata, raiz colapsa, slides)", "remove_D_before.txt", 40, "remove_D_after.txt");

    std::cout << "remove() — cobertura extra:\n";
    // Remove uma chave armazenada em um no INTERNO: substitui pelo sucessor em ordem.
    removeCase<3>("E: remove 50 (chave interna, sucessor)", "remove_E_before.txt", 50, "remove_E_after.txt");
    // Underflow corrigido emprestando do irmao da ESQUERDA (o da direita nao tem).
    removeCase<3>("F: remove 80 (empresta do irmao esquerdo)", "remove_F_before.txt", 80, "remove_F_after.txt");
    // Remover a ultima chave colapsa a arvore para vazia.
    removeCase<3>("G: remove 42 (ultima chave -> arvore vazia)", "remove_G_before.txt", 42, "remove_G_after.txt");
    // Casos no-op devem deixar a arvore intacta.
    removeCase<3>("H: remove 99 (chave ausente, no-op)", "remove_H_before.txt", 99, "remove_H_after.txt");
    removeCase<3>("I: remove de arvore vazia (no-op)", "remove_I_before.txt", 10, "remove_I_after.txt");
}

int main() {
    std::cout << "=== Testes da BTree com dados de mock (exemplos dos slides da aula) ===\n";
    runSearchTests();
    runInsertTests();
    runRemoveTests();

    std::cout << "------------------------------------------------------\n";
    std::cout << g_pass << " passaram, " << g_fail << " falharam\n";
    return g_fail == 0 ? 0 : 1;
}
