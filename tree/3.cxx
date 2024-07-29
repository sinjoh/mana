#include <vtkSmartPointer.h>
#include <vtkHyperTreeGrid.h>
#include <vtkHyperTreeGridSource.h>
#include <vtkHyperTreeGridNonOrientedCursor.h>
#include <vtkDoubleArray.h>
#include <vtkCellData.h>

void RefineCursor(vtkSmartPointer<vtkHyperTreeGridNonOrientedCursor> cursor, int levels, double refinementThreshold) {
    if (levels == 0) {
        return;
    }
    double cellValue = cursor->GetTree()->GetCellData()->GetScalars()->GetTuple1(cursor->GetGlobalNodeIndex());
    if (cellValue > refinementThreshold) {
        cursor->SubdivideLeaf();
        for (unsigned int child = 0; child < cursor->GetNumberOfChildren(); ++child) {
            cursor->ToChild(child);
            RefineCursor(cursor, levels - 1, refinementThreshold);
            cursor->ToParent();
        }
    }
}

void CoarsenCursor(vtkSmartPointer<vtkHyperTreeGridNonOrientedCursor> cursor, int levels, double coarseningThreshold) {
    if (levels == 0) {
        return;
    }
    if (!cursor->IsLeaf()) {
        for (unsigned int child = 0; child < cursor->GetNumberOfChildren(); ++child) {
            cursor->ToChild(child);
            CoarsenCursor(cursor, levels - 1, coarseningThreshold);
            cursor->ToParent();
        }
        double cellValue = cursor->GetTree()->GetCellData()->GetScalars()->GetTuple1(cursor->GetGlobalNodeIndex());
        if (cellValue < coarseningThreshold) {
            cursor->CollapseLeaf();
        }
    }
}

void BalanceGrid(vtkSmartPointer<vtkHyperTreeGrid> hyperTreeGrid) {
    bool balanced = false;
    while (!balanced) {
        balanced = true;
        vtkSmartPointer<vtkHyperTreeGridNonOrientedCursor> cursor =
            vtkSmartPointer<vtkHyperTreeGridNonOrientedCursor>::New();
        hyperTreeGrid->InitializeNonOrientedCursor(cursor, 0, true);

        // Traverse all cells
        vtkIdType numCells = hyperTreeGrid->GetNumberOfCells();
        for (vtkIdType cellId = 0; cellId < numCells; ++cellId) {
            cursor->InitializeNonOrientedCursor(cellId, true);
            int level = cursor->GetLevel();

            // Check neighbors
            for (int neighbor = 0; neighbor < cursor->GetNumberOfChildren(); ++neighbor) {
                if (cursor->ToChild(neighbor)) {
                    int neighborLevel = cursor->GetLevel();
                    if (abs(level - neighborLevel) > 1) {
                        balanced = false;
                        if (neighborLevel < level) {
                            RefineCursor(cursor, 1, 0.0); // Force refinement of the neighbor
                        } else {
                            cursor->CollapseLeaf(); // Coarsen current cell
                        }
                    }
                    cursor->ToParent();
                }
            }
        }
    }
}

int main(int argc, char* argv[])
{
    // Create a hyper tree grid source
    vtkSmartPointer<vtkHyperTreeGridSource> hyperTreeGridSource = 
        vtkSmartPointer<vtkHyperTreeGridSource>::New();
    hyperTreeGridSource->SetDimensions(4, 4, 4); // 3D grid with 4x4x4 cells
    hyperTreeGridSource->SetBranchFactor(2); // Each cell can be divided into 2x2x2 subcells

    // Update to create initial grid
    hyperTreeGridSource->Update();
    vtkSmartPointer<vtkHyperTreeGrid> hyperTreeGrid = hyperTreeGridSource->GetOutput();

    // Create a cell data array
    vtkSmartPointer<vtkDoubleArray> cellData = vtkSmartPointer<vtkDoubleArray>::New();
    cellData->SetName("CellData");
    cellData->SetNumberOfComponents(1);
    cellData->SetNumberOfTuples(hyperTreeGrid->GetNumberOfCells());

    // Initialize cell data values
    for (vtkIdType i = 0; i < hyperTreeGrid->GetNumberOfCells(); ++i) {
        cellData->SetValue(i, static_cast<double>(i));
    }

    // Add cell data to the grid
    hyperTreeGrid->GetCellData()->SetScalars(cellData);

    // Refine based on criteria
    double refinementThreshold = 10.0; // Example threshold for refining
    vtkSmartPointer<vtkHyperTreeGridNonOrientedCursor> cursor = 
        vtkSmartPointer<vtkHyperTreeGridNonOrientedCursor>::New();
    hyperTreeGrid->InitializeNonOrientedCursor(cursor, 0, true);

    RefineCursor(cursor, 3, refinementThreshold);

    // Coarsen based on criteria
    double coarseningThreshold = 20.0; // Example threshold for coarsening
    hyperTreeGrid->InitializeNonOrientedCursor(cursor, 0, true);
    CoarsenCursor(cursor, 3, coarseningThreshold);

    // Ensure 2:1 balance
    BalanceGrid(hyperTreeGrid);

    // Optionally, update cell data values after refinement and coarsening
    vtkIdType numCells = hyperTreeGrid->GetNumberOfCells();
    for (vtkIdType i = 0; i < numCells; ++i) {
        // Assign new values to cell data (e.g., based on some criteria)
        double newValue = static_cast<double>(i) * 2.0;
        cellData->SetValue(i, newValue);
    }

    // The grid is now refined, coarsened, balanced, and cell data is added. Any further processing can be done here.

    return EXIT_SUCCESS;
}
