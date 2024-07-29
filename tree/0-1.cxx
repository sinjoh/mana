#include <vtkSmartPointer.h>
#include <vtkRectilinearGrid.h>
#include <vtkUniformHyperTreeGrid.h>
#include <vtkDoubleArray.h>
#include <vtkCellData.h>
#include <vtkHyperTreeGridNonOrientedCursor.h>
#include <vtkRectilinearGridToHyperTreeGrid.h>

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

    // Now the hyper tree grid is initialized and cell data is copied. Any further processing can be done here.

    return EXIT_SUCCESS;
}
