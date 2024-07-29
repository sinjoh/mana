#include <vtkSmartPointer.h>
#include <vtkRectilinearGrid.h>
#include <vtkUniformHyperTreeGrid.h>
#include <vtkDoubleArray.h>
#include <vtkCellData.h>
#include <vtkHyperTreeGridNonOrientedCursor.h>
#include <vtkRectilinearGridToHyperTreeGrid.h>

bool CheckIfAllChildrenBelowThreshold(vtkSmartPointer<vtkHyperTreeGridNonOrientedCursor> cursor, double threshold) {
    bool allBelowThreshold = true;

    for (unsigned int child = 0; child < cursor->GetNumberOfChildren(); ++child) {
        cursor->ToChild(child);
        double childValue = cursor->GetTree()->GetCellData()->GetScalars()->GetTuple1(cursor->GetGlobalNodeIndex());
        if (childValue >= threshold) {
            allBelowThreshold = false;
        }
        cursor->ToParent();
    }
    return allBelowThreshold;
}

void CoarsenCursor(vtkSmartPointer<vtkHyperTreeGridNonOrientedCursor> cursor, double coarseningThreshold) {
    if (!cursor->IsLeaf()) {
        for (unsigned int child = 0; child < cursor->GetNumberOfChildren(); ++child) {
            cursor->ToChild(child);
            CoarsenCursor(cursor, coarseningThreshold);
            cursor->ToParent();
        }

        if (CheckIfAllChildrenBelowThreshold(cursor, coarseningThreshold)) {
            double parentValue = cursor->GetTree()->GetCellData()->GetScalars()->GetTuple1(cursor->GetGlobalNodeIndex());
            cursor->CollapseLeaf();
            cursor->GetTree()->GetCellData()->GetScalars()->SetTuple1(cursor->GetGlobalNodeIndex(), parentValue);
        }
    }
}

void InitializeHyperTreeGridFromRectilinearGrid(vtkSmartPointer<vtkRectilinearGrid> rectilinearGrid,
                                                vtkSmartPointer<vtkUniformHyperTreeGrid> hyperTreeGrid) {
    // Set dimensions
    int dimensions[3];
    rectilinearGrid->GetDimensions(dimensions);
    hyperTreeGrid->SetDimensions(dimensions);

    // Set grid bounds
    double bounds[6];
    rectilinearGrid->GetBounds(bounds);
    hyperTreeGrid->SetGridScale((bounds[1] - bounds[0]) / (dimensions[0] - 1),
                                (bounds[3] - bounds[2]) / (dimensions[1] - 1),
                                (bounds[5] - bounds[4]) / (dimensions[2] - 1));

    // Initialize the HyperTreeGrid
    hyperTreeGrid->Initialize();

    // Copy cell data arrays
    vtkCellData* rectilinearCellData = rectilinearGrid->GetCellData();
    vtkCellData* hyperTreeCellData = hyperTreeGrid->GetCellData();
    int numArrays = rectilinearCellData->GetNumberOfArrays();
    for (int i = 0; i < numArrays; ++i) {
        vtkDataArray* array = rectilinearCellData->GetArray(i);
        vtkSmartPointer<vtkDataArray> newArray = array->NewInstance();
        newArray->DeepCopy(array);
        hyperTreeCellData->AddArray(newArray);
    }

    // Copy cell data values to corresponding cells in HyperTreeGrid
    vtkSmartPointer<vtkHyperTreeGridNonOrientedCursor> cursor =
        vtkSmartPointer<vtkHyperTreeGridNonOrientedCursor>::New();
    hyperTreeGrid->InitializeNonOrientedCursor(cursor, 0, true);

    vtkIdType numCells = rectilinearGrid->GetNumberOfCells();
    for (vtkIdType cellId = 0; cellId < numCells; ++cellId) {
        cursor->InitializeNonOrientedCursor(cellId, true);
        for (int j = 0; j < numArrays; ++j) {
            vtkDataArray* rectArray = rectilinearCellData->GetArray(j);
            vtkDataArray* hyperArray = hyperTreeCellData->GetArray(j);
            double value = rectArray->GetTuple1(cellId);
            hyperArray->SetTuple1(cursor->GetGlobalNodeIndex(), value);
        }
    }
}

int main(int argc, char* argv[])
{
    // Create a rectilinear grid
    vtkSmartPointer<vtkRectilinearGrid> rectilinearGrid = vtkSmartPointer<vtkRectilinearGrid>::New();
    rectilinearGrid->SetDimensions(4, 4, 4); // 3D grid with 4x4x4 cells

    // Create coordinates for rectilinear grid
    vtkSmartPointer<vtkDoubleArray> xCoords = vtkSmartPointer<vtkDoubleArray>::New();
    vtkSmartPointer<vtkDoubleArray> yCoords = vtkSmartPointer<vtkDoubleArray>::New();
    vtkSmartPointer<vtkDoubleArray> zCoords = vtkSmartPointer<vtkDoubleArray>::New();
    for (int i = 0; i < 4; ++i) {
        xCoords->InsertNextValue(i);
        yCoords->InsertNextValue(i);
        zCoords->InsertNextValue(i);
    }
    rectilinearGrid->SetXCoordinates(xCoords);
    rectilinearGrid->SetYCoordinates(yCoords);
    rectilinearGrid->SetZCoordinates(zCoords);

    // Create and add cell data array
    vtkSmartPointer<vtkDoubleArray> cellData = vtkSmartPointer<vtkDoubleArray>::New();
    cellData->SetName("CellData");
    cellData->SetNumberOfComponents(1);
    cellData->SetNumberOfTuples(rectilinearGrid->GetNumberOfCells());
    for (vtkIdType i = 0; i < rectilinearGrid->GetNumberOfCells(); ++i) {
        cellData->SetValue(i, static_cast<double>(i % 25)); // Assign some value
    }
    rectilinearGrid->GetCellData()->AddArray(cellData);

    // Create a uniform hyper tree grid
    vtkSmartPointer<vtkUniformHyperTreeGrid> hyperTreeGrid = vtkSmartPointer<vtkUniformHyperTreeGrid>::New();

    // Initialize and copy cell data from rectilinear grid to hyper tree grid
    InitializeHyperTreeGridFromRectilinearGrid(rectilinearGrid, hyperTreeGrid);

    // Coarsen based on criteria
    double coarseningThreshold = 10.0; // Example threshold for coarsening
    vtkSmartPointer<vtkHyperTreeGridNonOrientedCursor> cursor =
        vtkSmartPointer<vtkHyperTreeGridNonOrientedCursor>::New();
    hyperTreeGrid->InitializeNonOrientedCursor(cursor, 0, true);
    CoarsenCursor(cursor, coarseningThreshold);

    // Optionally, update cell data values after coarsening
    vtkIdType numCells = hyperTreeGrid->GetNumberOfCells();
    for (vtkIdType i = 0; i < numCells; ++i) {
        // Assign new values to cell data (e.g., based on some criteria)
        double newValue = static_cast<double>(i) * 2.0;
        hyperTreeGrid->GetCellData()->GetScalars()->SetTuple1(i, newValue);
    }

    // Now the hyper tree grid is initialized and cell data is copied. Any further processing can be done here.

    return EXIT_SUCCESS;
}
