#include <vtkSmartPointer.h>
#include <vtkUnstructuredGrid.h>
#include <vtkPoints.h>
#include <vtkHexahedron.h>
#include <vtkSphere.h>
#include <vtkCellData.h>
#include <vtkUnsignedCharArray.h>
#include <vtkXMLUnstructuredGridWriter.h>
#include <vtkPointData.h>

int main(int argc, char* argv[])
{
    // Define grid dimensions
    int dims[3] = {100, 100, 100};
    double spacing[3] = {1.0, 1.0, 1.0};

    // Create a sphere
    vtkSmartPointer<vtkSphere> sphere = vtkSmartPointer<vtkSphere>::New();
    sphere->SetCenter(50.0, 50.0, 50.0);
    sphere->SetRadius(30.0);

    // Create points
    vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
    for (int z = 0; z < dims[2]; z++)
    {
        for (int y = 0; y < dims[1]; y++)
        {
            for (int x = 0; x < dims[0]; x++)
            {
                points->InsertNextPoint(x * spacing[0], y * spacing[1], z * spacing[2]);
            }
        }
    }

    // Create the unstructured grid
    vtkSmartPointer<vtkUnstructuredGrid> unstructuredGrid = vtkSmartPointer<vtkUnstructuredGrid>::New();
    unstructuredGrid->SetPoints(points);

    // Create and initialize scalar data array
    vtkSmartPointer<vtkUnsignedCharArray> scalars = vtkSmartPointer<vtkUnsignedCharArray>::New();
    scalars->SetNumberOfComponents(1);
    scalars->SetNumberOfTuples(dims[0] * dims[1] * dims[2]);

    // Create hexahedron cells
    for (int z = 0; z < dims[2] - 1; z++)
    {
        for (int y = 0; y < dims[1] - 1; y++)
        {
            for (int x = 0; x < dims[0] - 1; x++)
            {
                vtkSmartPointer<vtkHexahedron> hex = vtkSmartPointer<vtkHexahedron>::New();
                hex->GetPointIds()->SetId(0, x + y * dims[0] + z * dims[0] * dims[1]);
                hex->GetPointIds()->SetId(1, (x + 1) + y * dims[0] + z * dims[0] * dims[1]);
                hex->GetPointIds()->SetId(2, (x + 1) + (y + 1) * dims[0] + z * dims[0] * dims[1]);
                hex->GetPointIds()->SetId(3, x + (y + 1) * dims[0] + z * dims[0] * dims[1]);
                hex->GetPointIds()->SetId(4, x + y * dims[0] + (z + 1) * dims[0] * dims[1]);
                hex->GetPointIds()->SetId(5, (x + 1) + y * dims[0] + (z + 1) * dims[0] * dims[1]);
                hex->GetPointIds()->SetId(6, (x + 1) + (y + 1) * dims[0] + (z + 1) * dims[0] * dims[1]);
                hex->GetPointIds()->SetId(7, x + (y + 1) * dims[0] + (z + 1) * dims[0] * dims[1]);

                unstructuredGrid->InsertNextCell(hex->GetCellType(), hex->GetPointIds());
            }
        }
    }

    // Fill the scalar data array with sphere data
    for (int z = 0; z < dims[2]; z++)
    {
        for (int y = 0; y < dims[1]; y++)
        {
            for (int x = 0; x < dims[0]; x++)
            {
                double p[3] = {x * spacing[0], y * spacing[1], z * spacing[2]};
                double dist2 = sphere->EvaluateFunction(p);
                int index = z * dims[1] * dims[0] + y * dims[0] + x;
                scalars->SetValue(index, dist2 <= 0 ? 1 : 0);
            }
        }
    }

    // Add the scalar data to the grid
    unstructuredGrid->GetPointData()->SetScalars(scalars);

    // Create and initialize tag array
    vtkSmartPointer<vtkUnsignedCharArray> tagArray = vtkSmartPointer<vtkUnsignedCharArray>::New();
    tagArray->SetNumberOfComponents(1);
    tagArray->SetNumberOfTuples(unstructuredGrid->GetNumberOfCells());

    // Tag the interface cells using 26 neighbors
    for (vtkIdType cellId = 0; cellId < unstructuredGrid->GetNumberOfCells(); cellId++)
    {
        vtkCell* cell = unstructuredGrid->GetCell(cellId);
        bool isInterface = false;
        for (int i = 0; i < 8; i++)
        {
            double p[3];
            unstructuredGrid->GetPoint(cell->GetPointId(i), p);
            double dist2 = sphere->EvaluateFunction(p);
            if (dist2 <= 0)
            {
                for (int dz = -1; dz <= 1; dz++)
                {
                    for (int dy = -1; dy <= 1; dy++)
                    {
                        for (int dx = -1; dx <= 1; dx++)
                        {
                            if (dx == 0 && dy == 0 && dz == 0) continue; // Skip the center point
                            double np[3] = {p[0] + dx * spacing[0], p[1] + dy * spacing[1], p[2] + dz * spacing[2]};
                            if (sphere->EvaluateFunction(np) > 0)
                            {
                                isInterface = true;
                                break;
                            }
                        }
                        if (isInterface) break;
                    }
                    if (isInterface) break;
                }
            }
            if (isInterface) break;
        }
        tagArray->SetValue(cellId, isInterface ? 1 : 0);
    }

    // Add the tag array to the grid
    tagArray->SetName("Interface");
    unstructuredGrid->GetCellData()->AddArray(tagArray);

    // Save the unstructured grid data to file
    vtkSmartPointer<vtkXMLUnstructuredGridWriter> writer = vtkSmartPointer<vtkXMLUnstructuredGridWriter>::New();
    writer->SetFileName("output.vtu");
    writer->SetInputData(unstructuredGrid);
    writer->Write();

    return EXIT_SUCCESS;
}
