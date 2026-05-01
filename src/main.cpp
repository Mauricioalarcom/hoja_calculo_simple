#include "SparseMatrix.h"
#include "GUI.h"

int main() {
    SparseMatrix sheet;
    GUI gui(sheet);
    gui.run();
    return 0;
}