#include <vtkSmartPointer.h>
#include <vtkXMLUnstructuredGridReader.h>
#include <vtkUnstructuredGrid.h>
#include <vtkPoints.h>
#include <vtkCell.h>
#include <vtkCellData.h>
#include <vtkPointData.h>
#include <vtkDoubleArray.h>
#include <vtkDataArray.h>
#include <fstream>
#include <iostream>

void WriteAbaqusINP(vtkUnstructuredGrid* grid, const std::string& filename)
{
    std::ofstream outFile(filename);

    if (!outFile.is_open())
    {
        std::cerr << "Failed to open file " << filename << std::endl;
        return;
    }

    // Write header
    outFile << "*HEADING\n";
    outFile << "Converted from VTU to Abaqus INP\n";

    // Write nodes
    outFile << "*NODE\n";
    vtkPoints* points = grid->GetPoints();
    for (vtkIdType i = 0; i < points->GetNumberOfPoints(); ++i)
    {
        double p[3];
        points->GetPoint(i, p);
        outFile << i + 1 << ", " << p[0] << ", " << p[1] << ", " << p[2] << "\n";
    }

    // Write elements
    outFile << "*ELEMENT, TYPE=C3D4\n"; // Assuming tetrahedral elements
    for (vtkIdType i = 0; i < grid->GetNumberOfCells(); ++i)
    {
        vtkCell* cell = grid->GetCell(i);
        outFile << i + 1;
        for (vtkIdType j = 0; j < cell->GetNumberOfPoints(); ++j)
        {
            outFile << ", " << cell->GetPointId(j) + 1;
        }
        outFile << "\n";
    }

    // Write point data
    vtkPointData* pointData = grid->GetPointData();
    if (pointData)
    {
        for (int i = 0; i < pointData->GetNumberOfArrays(); ++i)
        {
            vtkDataArray* dataArray = pointData->GetArray(i);
            if (dataArray)
            {
                std::string dataName = dataArray->GetName();
                outFile << "*NODAL DATA, NAME=" << dataName << "\n";

                for (vtkIdType j = 0; j < dataArray->GetNumberOfTuples(); ++j)
                {
                    double value;
                    dataArray->GetTuple(j, &value);
                    outFile << j + 1 << ", " << value << "\n";
                }
            }
        }
    }

    // Write cell data
    vtkCellData* cellData = grid->GetCellData();
    if (cellData)
    {
        for (int i = 0; i < cellData->GetNumberOfArrays(); ++i)
        {
            vtkDataArray* dataArray = cellData->GetArray(i);
            if (dataArray)
            {
                std::string dataName = dataArray->GetName();
                outFile << "*ELSET, ELSET=" << dataName << "\n";

                for (vtkIdType j = 0; j < dataArray->GetNumberOfTuples(); ++j)
                {
                    double value;
                    dataArray->GetTuple(j, &value);
                    outFile << j + 1 << ", " << value << "\n";
                }
            }
        }
    }

    outFile.close();
}

int main(int argc, char* argv[])
{
    if (argc < 3)
    {
        std::cerr << "Usage: " << argv[0] << " input.vtu output.inp" << std::endl;
        return EXIT_FAILURE;
    }

    std::string inputFilename = argv[1];
    std::string outputFilename = argv[2];

    vtkSmartPointer<vtkXMLUnstructuredGridReader> reader = vtkSmartPointer<vtkXMLUnstructuredGridReader>::New();
    reader->SetFileName(inputFilename.c_str());
    reader->Update();

    vtkUnstructuredGrid* grid = reader->GetOutput();
    WriteAbaqusINP(grid, outputFilename);

    return EXIT_SUCCESS;
}
