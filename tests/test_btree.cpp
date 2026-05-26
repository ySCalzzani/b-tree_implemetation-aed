/* ----------------------------------------------------------------------------
 * test_btree.cpp
 * Mock-data tests for BTree<M>, focused on insert() and search().
 *
 * Idea (see git history / loadFromFile): describe a "sample" tree and the
 * expected "final" tree as readable text fixtures, run an operation on the
 * sample, and compare the result against the final tree record by record.
 *
 * btree.h is header-only, so this file has its own main() and is compiled
 * standalone (it never links src/main.o). Run with `make test`.
 *
 * NOTE: insert() still relies on the stubs allocateNode() and split(), so the
 * empty-tree and split cases below are expected to FAIL until those are
 * implemented. Their diffs are the spec for that work.
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

static long fileSize(const std::string& path) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f.is_open()) return -1;
    return (long)f.tellg();
}

/* Exact, record-by-record comparison of two BTree<M> data files.
 * Compares numKeys, every K[i] and every A[i] (pointer IDs included).
 * Sizes are checked first, so a corrupt/runaway file is reported cheaply
 * without reading the whole thing. */
template <int M>
static bool compareExact(const std::string& a, const std::string& b, std::string& msg) {
    long sa = fileSize(a);
    long sb = fileSize(b);
    if (sa < 0 || sb < 0) { msg = "could not open data file(s)"; return false; }
    if (sa != sb) {
        msg = "file size differs (actual=" + std::to_string(sa) +
              " bytes, expected=" + std::to_string(sb) + " bytes)";
        return false;
    }

    std::ifstream fa(a, std::ios::binary);
    std::ifstream fb(b, std::ios::binary);
    const long recSize = (long)sizeof(BTreeNode<M>);
    const long nRec = sa / recSize;

    for (long i = 0; i < nRec; i++) {
        BTreeNode<M> na;
        BTreeNode<M> nb;
        fa.read((char*)&na, recSize);
        fb.read((char*)&nb, recSize);

        if (na.numKeys != nb.numKeys) {
            msg = "record " + std::to_string(i) + ": numKeys " +
                  std::to_string(na.numKeys) + " != " + std::to_string(nb.numKeys);
            return false;
        }
        for (int k = 0; k <= M; k++) {
            if (na.K[k] != nb.K[k]) {
                msg = "record " + std::to_string(i) + ": K[" + std::to_string(k) + "] " +
                      std::to_string(na.K[k]) + " != " + std::to_string(nb.K[k]);
                return false;
            }
            if (na.A[k] != nb.A[k]) {
                msg = "record " + std::to_string(i) + ": A[" + std::to_string(k) + "] " +
                      std::to_string(na.A[k]) + " != " + std::to_string(nb.A[k]);
                return false;
            }
        }
    }
    return true;
}

/* Loads a fixture into a fresh .dat file and closes it (flush on destructor). */
template <int M>
static void materialize(const std::string& dat, const std::string& fixture) {
    std::remove(dat.c_str());
    BTree<M> t(dat);
    t.loadFromFile(FIX + fixture);
}

/* sample fixture -> insert(key) -> compare against expected fixture.
 *
 * The load+insert runs in a forked child: insert() currently leans on the
 * unimplemented stubs allocateNode()/split(), and a buggy stub can crash
 * (e.g. allocateNode() falls off the end -> SIGILL). Forking turns such a
 * crash into a single reported FAIL instead of aborting the whole suite. */
template <int M>
static void runInsertCase(const std::string& name,
                          const std::string& before,
                          int key,
                          const std::string& after) {
    const std::string actual = TMP + "test_actual.dat";
    const std::string expected = TMP + "test_expected.dat";

    std::remove(actual.c_str());
    std::cout.flush();  // don't let the child inherit unflushed output

    pid_t pid = fork();
    if (pid == 0) {
        // Child: perform the operation, flush via destructor, then _exit
        // (which skips stdio flushing, so it can't duplicate our buffer).
        {
            BTree<M> t(actual);
            t.loadFromFile(FIX + before);
            t.insert(key);
        }
        _exit(0);
    }

    int status = 0;
    waitpid(pid, &status, 0);
    if (WIFSIGNALED(status)) {
        check(name, false, "operation crashed (signal " +
                           std::to_string(WTERMSIG(status)) +
                           ") — allocateNode()/split() not implemented yet");
        return;
    }

    materialize<M>(expected, after);

    std::string msg;
    bool ok = compareExact<M>(actual, expected, msg);
    check(name, ok, msg);
}

static void runSearchTests() {
    std::cout << "search():\n";
    const std::string dat = TMP + "test_search.dat";
    std::remove(dat.c_str());

    BTree<3> t(dat);
    t.loadFromFile(FIX + "search_tree.txt");

    // Present keys, at every level of the tree.
    check("find 20 (root key)",   t.search(20));
    check("find 40 (root key)",   t.search(40));
    check("find 10 (left leaf)",  t.search(10));
    check("find 30 (mid leaf)",   t.search(30));
    check("find 50 (right leaf)", t.search(50));

    // Absent keys: below, above, and between existing ones.
    check("miss 5 (below all)",   !t.search(5));
    check("miss 99 (above all)",  !t.search(99));
    check("miss 33 (gap)",        !t.search(33));
}

static void runInsertTests() {
    std::cout << "insert():\n";
    // Green today (no allocateNode/split needed):
    runInsertCase<3>("A: no-split insert", "insert_A_nosplit_before.txt", 20, "insert_A_nosplit_after.txt");
    runInsertCase<3>("B: duplicate key (no-op)", "insert_B_dup_before.txt", 10, "insert_B_dup_after.txt");

    // RED until the stubs are implemented (spec for that work):
    runInsertCase<3>("C: insert into empty tree", "insert_C_empty_before.txt", 50, "insert_C_empty_after.txt");
    runInsertCase<3>("D: leaf/root split", "insert_D_split_before.txt", 30, "insert_D_split_after.txt");
}

int main() {
    std::cout << "=== BTree mock-data tests ===\n";
    runSearchTests();
    runInsertTests();

    std::cout << "-----------------------------\n";
    std::cout << g_pass << " passed, " << g_fail << " failed\n";
    return g_fail == 0 ? 0 : 1;
}
