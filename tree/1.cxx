#include <vtkSmartPointer.h>
#include <vtkHyperTreeGrid.h>
#include <vtkHyperTreeGridSource.h>
#include <vtkHyperTreeGridNonOrientedCursor.h>
#include <vtkDoubleArray.h>
#include <vtkHyperTreeGridNonOrientedGeometryCursor.h>
#include <vtkCellData.h>

void RefineCursor(vtkSmartPointer<vtkHyperTreeGridNonOrientedCursor> cursor, int levels) {
    if (levels == 0) {
        return;
    }
    cursor->SubdivideLeaf();
    for (unsigned int child = 0; child < cursor->GetNumberOfChildren(); ++child) {
        cursor->ToChild(child);
        RefineCursor(cursor, levels - 1);
        cursor->ToParent();
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

    // Refine uniformly for three levels
    vtkSmartPointer<vtkHyperTreeGridNonOrientedCursor> cursor = 
        vtkSmartPointer<vtkHyperTreeGridNonOrientedCursor>::New();
    hyperTreeGrid->InitializeNonOrientedCursor(cursor, 0, true);

    RefineCursor(cursor, 3);

    // Optionally, update cell data values after refinement
    vtkIdType numCells = hyperTreeGrid->GetNumberOfCells();
    for (vtkIdType i = 0; i < numCells; ++i) {
        // Assign new values to cell data (e.g., based on some criteria)
        double newValue = static_cast<double>(i) * 2.0;
        cellData->SetValue(i, newValue);
    }

    // The grid is now refined and cell data is added. Any further processing can be done here.

    return EXIT_SUCCESS;
}
