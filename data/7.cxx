#include <vtkSmartPointer.h>
#include <vtkRectilinearGrid.h>
#include <vtkSphere.h>
#include <vtkUnsignedCharArray.h>
#include <vtkXMLRectilinearGridWriter.h>
#include <vtkDoubleArray.h>
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

    // Create a rectilinear grid
    vtkSmartPointer<vtkRectilinearGrid> rectilinearGrid = vtkSmartPointer<vtkRectilinearGrid>::New();
    rectilinearGrid->SetDimensions(dims);

    vtkSmartPointer<vtkDoubleArray> xCoords = vtkSmartPointer<vtkDoubleArray>::New();
    vtkSmartPointer<vtkDoubleArray> yCoords = vtkSmartPointer<vtkDoubleArray>::New();
    vtkSmartPointer<vtkDoubleArray> zCoords = vtkSmartPointer<vtkDoubleArray>::New();
    for (int i = 0; i < dims[0]; i++) xCoords->InsertNextValue(i * spacing[0]);
    for (int i = 0; i < dims[1]; i++) yCoords->InsertNextValue(i * spacing[1]);
    for (int i = 0; i < dims[2]; i++) zCoords->InsertNextValue(i * spacing[2]);
    rectilinearGrid->SetXCoordinates(xCoords);
    rectilinearGrid->SetYCoordinates(yCoords);
    rectilinearGrid->SetZCoordinates(zCoords);

    // Create and initialize scalar data array
    vtkSmartPointer<vtkUnsignedCharArray> scalars = vtkSmartPointer<vtkUnsignedCharArray>::New();
    scalars->SetName("Scalars");
    scalars->SetNumberOfComponents(1);
    scalars->SetNumberOfTuples(dims[0] * dims[1] * dims[2]);
    for (int i = 0; i < dims[0] * dims[1] * dims[2]; i++) scalars->SetValue(i, 0);

    // Fill the rectilinear grid with sphere data
    for (int z = 0; z < dims[2]; z++)
    {
        for (int y = 0; y < dims[1]; y++)
        {
            for (int x = 0; x < dims[0]; x++)
            {
                double p[3] = {x * spacing[0], y * spacing[1], z * spacing[2]};
                double dist2 = sphere->EvaluateFunction(p);
                if (dist2 <= 0)
                {
                    int index = z * dims[1] * dims[0] + y * dims[0] + x;
                    scalars->SetValue(index, 1);
                }
            }
        }
    }

    rectilinearGrid->GetPointData()->SetScalars(scalars);

    // Create and initialize tag array
    vtkSmartPointer<vtkUnsignedCharArray> tagArray = vtkSmartPointer<vtkUnsignedCharArray>::New();
    tagArray->SetName("Interface");
    tagArray->SetNumberOfComponents(1);
    tagArray->SetNumberOfTuples(dims[0] * dims[1] * dims[2]);
    for (int i = 0; i < dims[0] * dims[1] * dims[2]; i++) tagArray->SetValue(i, 0);

    // Tag the interface cells and their neighbors using 26 neighbors
    for (int z = 0; z < dims[2]; z++)
    {
        for (int y = 0; y < dims[1]; y++)
        {
            for (int x = 0; x < dims[0]; x++)
            {
                int index = z * dims[1] * dims[0] + y * dims[0] + x;
                if (scalars->GetValue(index) == 1)
                {
                    bool isInterface = false;
                    for (int dz = -1; dz <= 1; dz++)
                    {
                        for (int dy = -1; dy <= 1; dy++)
                        {
                            for (int dx = -1; dx <= 1; dx++)
                            {
                                if (dx == 0 && dy == 0 && dz == 0) continue; // Skip the center point
                                int nx = x + dx;
                                int ny = y + dy;
                                int nz = z + dz;
                                if (nx >= 0 && nx < dims[0] && ny >= 0 && ny < dims[1] && nz >= 0 && nz < dims[2])
                                {
                                    int neighborIndex = nz * dims[1] * dims[0] + ny * dims[0] + nx;
                                    if (scalars->GetValue(neighborIndex) == 0)
                                    {
                                        isInterface = true;
                                        break;
                                    }
                                }
                            }
                            if (isInterface) break;
                        }
                        if (isInterface) break;
                    }
                    if (isInterface)
                    {
                        tagArray->SetValue(index, 1);
                        // Tag neighboring cells as well
                        for (int dz = -1; dz <= 1; dz++)
                        {
                            for (int dy = -1; dy <= 1; dy++)
                            {
                                for (int dx = -1; dx <= 1; dx++)
                                {
                                    int nx = x + dx;
                                    int ny = y + dy;
                                    int nz = z + dz;
                                    if (nx >= 0 && nx < dims[0] && ny >= 0 && ny < dims[1] && nz >= 0 && nz < dims[2])
                                    {
                                        int neighborIndex = nz * dims[1] * dims[0] + ny * dims[0] + nx;
                                        tagArray->SetValue(neighborIndex, 1);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    rectilinearGrid->GetCellData()->AddArray(tagArray);

    // Save the rectilinear grid data to file
    vtkSmartPointer<vtkXMLRectilinearGridWriter> writer = vtkSmartPointer<vtkXMLRectilinearGridWriter>::New();
    writer->SetFileName("output.vtr");
    writer->SetInputData(rectilinearGrid);
    writer->Write();

    return EXIT_SUCCESS;
}
