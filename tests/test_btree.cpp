/* ----------------------------------------------------------------------------
 * test_btree.cpp
 * Mock-data tests for BTree<M>, covering search(), insert() and remove().
 *
 * Each case describes the "before" and expected "final" tree as a readable
 * text fixture (tests/fixtures/*.txt) loaded by BTree::loadFromFile(). The
 * operation runs on the before-tree and the result is compared against the
 * final tree.
 *
 * Comparison is LOGICAL: we walk both trees from the root following child
 * pointers, comparing each node's key count and keys and the subtree shape.
 * Physical record ids and abandoned-on-disk nodes are ignored — the remove
 * slides explicitly leave dead nodes behind, so a byte/record compare would
 * be ill-defined.
 *
 * btree.h is header-only, so this file has its own main() and is compiled
 * standalone (it never links src/main.o). Run with `make test`.
 *
 * NOTE: insert() still depends on the stubs allocateNode()/split(), and
 * remove() is an empty stub, so most insert/remove cases are EXPECTED TO FAIL
 * until those are implemented. Their diffs/crashes are the spec for that work.
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

/* Recursively compare the subtrees rooted at records idA / idB. A child id of
 * 0 means "no child"; both sides must agree on that. */
template <int M>
static bool compareSubtree(std::ifstream& fa, int idA,
                           std::ifstream& fb, int idB,
                           std::string& msg) {
    if (idA == 0 && idB == 0) return true;
    if (idA == 0 || idB == 0) {
        msg = "tree shape differs (one child is null, the other is not)";
        return false;
    }

    BTreeNode<M> na;
    BTreeNode<M> nb;
    readRecord(fa, idA, na);
    readRecord(fb, idB, nb);

    if (na.numKeys < 0 || na.numKeys > M) {
        msg = "invalid numKeys " + std::to_string(na.numKeys) + " in result tree";
        return false;
    }
    if (na.numKeys != nb.numKeys) {
        msg = "numKeys differs: result has " + std::to_string(na.numKeys) +
              ", expected " + std::to_string(nb.numKeys);
        return false;
    }
    for (int k = 1; k <= na.numKeys; k++) {
        if (na.K[k] != nb.K[k]) {
            msg = "key differs: result has " + std::to_string(na.K[k]) +
                  ", expected " + std::to_string(nb.K[k]);
            return false;
        }
    }
    for (int i = 0; i <= na.numKeys; i++) {
        if (!compareSubtree<M>(fa, na.A[i], fb, nb.A[i], msg)) return false;
    }
    return true;
}

/* Logical comparison of two BTree<M> data files: same keys, same shape,
 * starting from each file's root (record 0 header, A[0] = root id). */
template <int M>
static bool compareLogical(const std::string& a, const std::string& b, std::string& msg) {
    std::ifstream fa(a, std::ios::binary);
    std::ifstream fb(b, std::ios::binary);
    if (!fa.is_open() || !fb.is_open()) {
        msg = "could not open data file(s)";
        return false;
    }
    BTreeNode<M> ha;
    BTreeNode<M> hb;
    readRecord(fa, 0, ha);
    readRecord(fb, 0, hb);
    return compareSubtree<M>(fa, ha.A[0], fb, hb.A[0], msg);
}

/* Loads a fixture into a fresh .dat file and closes it (flush on destructor). */
template <int M>
static void materialize(const std::string& dat, const std::string& fixture) {
    std::remove(dat.c_str());
    BTree<M> t(dat);
    t.loadFromFile(FIX + fixture);
}

/* before fixture -> run op(tree) -> compare against the expected fixture.
 *
 * The load+op runs in a forked child: insert() leans on the unimplemented
 * stubs allocateNode()/split(), and a buggy stub can crash (e.g. allocateNode
 * falling off the end -> SIGILL). Forking turns such a crash into a single
 * reported FAIL instead of aborting the whole suite. */
template <int M, typename Op>
static void runCase(const std::string& name,
                    const std::string& before,
                    Op op,
                    const std::string& after) {
    const std::string actual = TMP + "test_actual.dat";
    const std::string expected = TMP + "test_expected.dat";

    std::remove(actual.c_str());
    std::cout.flush();  // don't let the child inherit unflushed output

    pid_t pid = fork();
    if (pid == 0) {
        {
            BTree<M> t(actual);
            t.loadFromFile(FIX + before);
            op(t);
        }  // destructor flushes/closes before the parent reads the file
        _exit(0);
    }

    int status = 0;
    waitpid(pid, &status, 0);
    if (WIFSIGNALED(status)) {
        check(name, false, "operation crashed (signal " +
                           std::to_string(WTERMSIG(status)) +
                           ") — not implemented yet");
        return;
    }

    materialize<M>(expected, after);

    std::string msg;
    bool ok = compareLogical<M>(actual, expected, msg);
    check(name, ok, msg);
}

/* Convenience wrappers so each case reads as one line. */
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
    std::cout << "search() — slide-60 B-tree:\n";
    const std::string dat = TMP + "test_search.dat";
    std::remove(dat.c_str());

    BTree<3> t(dat);
    t.loadFromFile(FIX + "search_slide60.txt");

    // Present keys at every level.
    check("find 30 (root)",        t.search(30));
    check("find 20 (internal b)",  t.search(20));
    check("find 40 (internal c)",  t.search(40));
    check("find 10 (leaf d)",      t.search(10));
    check("find 25 (leaf e)",      t.search(25));
    check("find 35 (leaf f)",      t.search(35));
    check("find 50 (leaf g)",      t.search(50));

    // Absent keys: below, above and between existing ones.
    check("miss 5 (below all)",    !t.search(5));
    check("miss 33 (gap)",         !t.search(33));
    check("miss 99 (above all)",   !t.search(99));
}

static void runInsertTests() {
    std::cout << "insert():\n";
    // Green today (no allocateNode/split needed):
    insertCase<3>("A: no-split insert", "insert_A_nosplit_before.txt", 20, "insert_A_nosplit_after.txt");
    insertCase<3>("B: duplicate key (no-op)", "insert_B_dup_before.txt", 10, "insert_B_dup_after.txt");

    // RED until allocateNode()/split() are implemented:
    insertCase<3>("C: insert into empty tree", "insert_C_empty_before.txt", 50, "insert_C_empty_after.txt");
    insertCase<3>("D: root-leaf split", "insert_D_split_before.txt", 30, "insert_D_split_after.txt");
    insertCase<3>("slide: insert 55 (split + promote)", "insert_slide55_before.txt", 55, "insert_slide55_after.txt");
}

static void runRemoveTests() {
    std::cout << "remove() — slide sequence (RED until remove() is implemented):\n";
    removeCase<3>("slide: remove 58 (simple leaf)",        "remove_slide_start.txt",    58, "remove_slide_after_58.txt");
    removeCase<3>("slide: remove 65 (borrow from sibling)", "remove_slide_after_58.txt", 65, "remove_slide_after_65.txt");
    removeCase<3>("slide: remove 55 (merge with sibling)",  "remove_slide_after_65.txt", 55, "remove_slide_after_55.txt");
    removeCase<3>("slide: remove 40 (cascading merge, root collapses)", "remove_slide_after_55.txt", 40, "remove_slide_after_40.txt");
}

int main() {
    std::cout << "=== BTree mock-data tests (lecture-slide examples) ===\n";
    runSearchTests();
    runInsertTests();
    runRemoveTests();

    std::cout << "------------------------------------------------------\n";
    std::cout << g_pass << " passed, " << g_fail << " failed\n";
    return g_fail == 0 ? 0 : 1;
}
