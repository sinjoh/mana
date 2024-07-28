#include <vtkSmartPointer.h>
#include <vtkImageData.h>
#include <vtkSphere.h>
#include <vtkImageStencil.h>
#include <vtkPolyDataToImageStencil.h>
#include <vtkImageCast.h>
#include <vtkUnsignedCharArray.h>
#include <vtkUnstructuredGrid.h>
#include <vtkPoints.h>
#include <vtkHexahedron.h>
#include <vtkXMLUnstructuredGridWriter.h>
#include <vtkPointData.h>
#include <vtkCellData.h>

int main(int argc, char* argv[])
{
    // Define grid dimensions
    int dims[3] = {100, 100, 100};
    double spacing[3] = {1.0, 1.0, 1.0};

    // Create a sphere
    vtkSmartPointer<vtkSphere> sphere = vtkSmartPointer<vtkSphere>::New();
    sphere->SetCenter(50.0, 50.0, 50.0);
    sphere->SetRadius(30.0);

    // Create image data
    vtkSmartPointer<vtkImageData> imageData = vtkSmartPointer<vtkImageData>::New();
    imageData->SetDimensions(dims);
    imageData->AllocateScalars(VTK_UNSIGNED_CHAR, 1);

    // Initialize the image data to 0
    for (int z = 0; z < dims[2]; z++)
    {
        for (int y = 0; y < dims[1]; y++)
        {
            for (int x = 0; x < dims[0]; x++)
            {
                unsigned char* pixel = static_cast<unsigned char*>(imageData->GetScalarPointer(x, y, z));
                pixel[0] = 0;
            }
        }
    }

    // Fill the image data with the sphere
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
                    unsigned char* pixel = static_cast<unsigned char*>(imageData->GetScalarPointer(x, y, z));
                    pixel[0] = 1;
                }
            }
        }
    }

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

    // Add the scalar data to the grid
    unstructuredGrid->GetPointData()->SetScalars(imageData->GetPointData()->GetScalars());

    // Create and initialize tag array
    vtkSmartPointer<vtkUnsignedCharArray> tagArray = vtkSmartPointer<vtkUnsignedCharArray>::New();
    tagArray->SetNumberOfComponents(1);
    tagArray->SetNumberOfTuples(unstructuredGrid->GetNumberOfCells());

    // Tag the interface cells using 26 neighbors and their neighbors
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
        if (isInterface)
        {
            tagArray->SetValue(cellId, 1);
            // Also tag neighboring cells
            for (int dz = -1; dz <= 1; dz++)
            {
                for (int dy = -1; dy <= 1; dy++)
                {
                    for (int dx = -1; dx <= 1; dx++)
                    {
                        if (dx == 0 && dy == 0 && dz == 0) continue; // Skip the center point
                        double p[3];
                        unstructuredGrid->GetPoint(cell->GetPointId(0), p); // Use the first point of the cell as reference
                        double np[3] = {p[0] + dx * spacing[0], p[1] + dy * spacing[1], p[2] + dz * spacing[2]};
                        int nx = (int)(np[0] / spacing[0]);
                        int ny = (int)(np[1] / spacing[1]);
                        int nz = (int)(np[2] / spacing[2]);
                        if (nx >= 0 && nx < dims[0] - 1 && ny >= 0 && ny < dims[1] - 1 && nz >= 0 && nz < dims[2] - 1)
                        {
                            int neighborCellId = nx + ny * (dims[0] - 1) + nz * (dims[0] - 1) * (dims[1] - 1);
                            tagArray->SetValue(neighborCellId, 1);
                        }
                    }
                }
            }
        }
        else
        {
            tagArray->SetValue(cellId, 0);
        }
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
