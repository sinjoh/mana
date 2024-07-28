#include <vtkSmartPointer.h>
#include <vtkImageData.h>
#include <vtkSphere.h>
#include <vtkImageStencil.h>
#include <vtkPolyDataToImageStencil.h>
#include <vtkImageCast.h>
#include <vtkXMLImageDataWriter.h>
#include <vtkPointData.h>
#include <vtkDoubleArray.h>

int main(int argc, char* argv[])
{
    // Create a sphere
    vtkSmartPointer<vtkSphere> sphere = vtkSmartPointer<vtkSphere>::New();
    sphere->SetCenter(50, 50, 50);
    sphere->SetRadius(30);

    // Create an image data
    vtkSmartPointer<vtkImageData> imageData = vtkSmartPointer<vtkImageData>::New();
    imageData->SetDimensions(100, 100, 100);
    imageData->AllocateScalars(VTK_UNSIGNED_CHAR, 1);

    // Initialize the image data to 0
    int* dims = imageData->GetDimensions();
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

    // Convert the sphere to stencil
    vtkSmartPointer<vtkPolyDataToImageStencil> polyDataToImageStencil = vtkSmartPointer<vtkPolyDataToImageStencil>::New();
    polyDataToImageStencil->SetInputData(sphere->GetOutput());

    // Create a stencil
    vtkSmartPointer<vtkImageStencil> imageStencil = vtkSmartPointer<vtkImageStencil>::New();
    imageStencil->SetInputData(imageData);
    imageStencil->SetStencilConnection(polyDataToImageStencil->GetOutputPort());
    imageStencil->ReverseStencilOff();
    imageStencil->SetBackgroundValue(1);
    imageStencil->Update();

    // Cast the image to unsigned char
    vtkSmartPointer<vtkImageCast> imageCast = vtkSmartPointer<vtkImageCast>::New();
    imageCast->SetInputConnection(imageStencil->GetOutputPort());
    imageCast->SetOutputScalarTypeToUnsignedChar();
    imageCast->Update();

    // Create a tag array for interface cells
    vtkSmartPointer<vtkImageData> tagImageData = vtkSmartPointer<vtkImageData>::New();
    tagImageData->SetDimensions(100, 100, 100);
    tagImageData->AllocateScalars(VTK_UNSIGNED_CHAR, 1);

    // Initialize the tag image data to 0
    for (int z = 0; z < dims[2]; z++)
    {
        for (int y = 0; y < dims[1]; y++)
        {
            for (int x = 0; x < dims[0]; x++)
            {
                unsigned char* tagPixel = static_cast<unsigned char*>(tagImageData->GetScalarPointer(x, y, z));
                tagPixel[0] = 0;
            }
        }
    }

    // Tag the interface cells using Cartesian neighbors
    for (int z = 0; z < dims[2]; z++)
    {
        for (int y = 0; y < dims[1]; y++)
        {
            for (int x = 0; x < dims[0]; x++)
            {
                unsigned char* pixel = static_cast<unsigned char*>(imageCast->GetOutput()->GetScalarPointer(x, y, z));
                unsigned char* tagPixel = static_cast<unsigned char*>(tagImageData->GetScalarPointer(x, y, z));
                if (pixel[0] == 1)
                {
                    // Check if any Cartesian neighboring voxel is zero
                    bool isInterface = false;
                    int neighborCoords[6][3] = {
                        {x - 1, y, z}, {x + 1, y, z}, 
                        {x, y - 1, z}, {x, y + 1, z}, 
                        {x, y, z - 1}, {x, y, z + 1}
                    };

                    for (int i = 0; i < 6; i++)
                    {
                        int nx = neighborCoords[i][0];
                        int ny = neighborCoords[i][1];
                        int nz = neighborCoords[i][2];
                        if (nx >= 0 && nx < dims[0] && ny >= 0 && ny < dims[1] && nz >= 0 && nz < dims[2])
                        {
                            unsigned char* neighborPixel = static_cast<unsigned char*>(imageCast->GetOutput()->GetScalarPointer(nx, ny, nz));
                            if (neighborPixel[0] == 0)
                            {
                                isInterface = true;
                                break;
                            }
                        }
                    }
                    tagPixel[0] = isInterface ? 1 : 0;
                }
                else
                {
                    tagPixel[0] = 0;
                }
            }
        }
    }

    // Save the image and tag data to files
    vtkSmartPointer<vtkXMLImageDataWriter> writer = vtkSmartPointer<vtkXMLImageDataWriter>::New();
    writer->SetFileName("output.vti");
    writer->SetInputData(imageCast->GetOutput());
    writer->Write();

    vtkSmartPointer<vtkXMLImageDataWriter> tagWriter = vtkSmartPointer<vtkXMLImageDataWriter>::New();
    tagWriter->SetFileName("tag.vti");
    tagWriter->SetInputData(tagImageData);
    tagWriter->Write();

    return EXIT_SUCCESS;
}
