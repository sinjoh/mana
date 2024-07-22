#include <iostream>
#include <vtkSmartPointer.h>
#include <vtkXMLUnstructuredGridReader.h>
#include <vtkUnstructuredGrid.h>
#include <vtkXMLUnstructuredGridWriter.h>
#include <vtkPoints.h>
#include <vtkCellData.h>
#include <vtkIntArray.h>
#include <p4est.h>
#include <p4est_connectivity.h>

bool is_interface_cell(vtkUnstructuredGrid* grid, vtkIdType cellId, vtkIntArray* ids) {
    vtkCell* cell = grid->GetCell(cellId);
    int cellIdValue = ids->GetValue(cellId);

    for (vtkIdType i = 0; i < cell->GetNumberOfPoints(); ++i) {
        vtkIdList* neighborCells = vtkIdList::New();
        grid->GetPointCells(cell->GetPointId(i), neighborCells);
        for (vtkIdType j = 0; j < neighborCells->GetNumberOfIds(); ++j) {
            vtkIdType neighborCellId = neighborCells->GetId(j);
            if (neighborCellId != cellId && ids->GetValue(neighborCellId) != cellIdValue) {
                return true;
            }
        }
    }
    return false;
}

void refine_grid(p4est_t *p4est, int num_refinements) {
    for (int i = 0; i < num_refinements; ++i) {
        p4est_refine(p4est, 1, NULL, NULL);
    }
}

void coarsen_grid(p4est_t *p4est, int num_coarsenings) {
    for (int i = 0; i < num_coarsenings; ++i) {
        p4est_coarsen(p4est, 1, NULL, NULL);
    }
}

int main() {
    vtkSmartPointer<vtkXMLUnstructuredGridReader> reader = vtkSmartPointer<vtkXMLUnstructuredGridReader>::New();
    reader->SetFileName("sphere_cells.vtu");
    reader->Update();

    vtkSmartPointer<vtkUnstructuredGrid> grid = reader->GetOutput();
    vtkSmartPointer<vtkPoints> points = grid->GetPoints();
    vtkSmartPointer<vtkIntArray> ids = vtkIntArray::SafeDownCast(grid->GetCellData()->GetArray("RegionId"));

    if (!points || !ids) {
        std::cerr << "Error: Missing points or RegionId array in VTU file." << std::endl;
        return -1;
    }

    // Initialize p4est
    p4est_connectivity_t *conn = p4est_connectivity_new_unitsquare();
    p4est_t *p4est = p4est_new_ext(NULL, conn, 0, 1, 0, sizeof(int), NULL, NULL);

    // Refine the grid
    refine_grid(p4est, 5); // Refine to 5 levels

    // Coarsen the grid if the cell is not on the interface
    for (int i = 0; i < grid->GetNumberOfCells(); ++i) {
        if (!is_interface_cell(grid, i, ids)) {
            coarsen_grid(p4est, 1); // Initial coarsening
        }
    }

    // Further coarsen by 2 more levels
    for (int i = 0; i < 2; ++i) {
        coarsen_grid(p4est, 1);
    }

    // Extract refined points (example)
    vtkSmartPointer<vtkPoints> refined_points = vtkSmartPointer<vtkPoints>::New();
    vtkSmartPointer<vtkIntArray> refined_ids = vtkSmartPointer<vtkIntArray>::New();
    refined_ids->SetName("RegionId");

    for (p4est_topidx_t tree = 0; tree < p4est->local_num_trees; ++tree) {
        p4est_tree_t *ptree = &p4est->trees[tree];
        for (p4est_quadrant_t *quad = p4est_tree_quadrants(ptree); quad != NULL; quad = p4est_quadrant_next(quad)) {
            double x = (quad->x + 0.5) * conn->length_array[0];
            double y = (quad->y + 0.5) * conn->length_array[1];
            double z = 0.0; // Assuming 2D for this example
            vtkIdType id = refined_points->InsertNextPoint(x, y, z);
            refined_ids->InsertNextValue(0); // Adjust ID logic as necessary
        }
    }

    vtkSmartPointer<vtkUnstructuredGrid> refined_grid = vtkSmartPointer<vtkUnstructuredGrid>::New();
    refined_grid->SetPoints(refined_points);
    refined_grid->GetCellData()->AddArray(refined_ids);

    vtkSmartPointer<vtkXMLUnstructuredGridWriter> writer = vtkSmartPointer<vtkXMLUnstructuredGridWriter>::New();
    writer->SetFileName("refined_coarsened_sphere_cells.vtu");
    writer->SetInputData(refined_grid);
    writer->Write();

    p4est_destroy(p4est);
    p4est_connectivity_destroy(conn);

    return 0;
}
