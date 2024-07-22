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

void refine_grid(p4est_t *p4est, int num_refinements) {
    for (int i = 0; i < num_refinements; ++i) {
        p4est_refine(p4est, 1, NULL, NULL);
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

    // Refine the grid twice more
    refine_grid(p4est, 5); // Original 3 levels + 2 more levels = 5 levels of refinement

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
    writer->SetFileName("refined_sphere_cells.vtu");
    writer->SetInputData(refined_grid);
    writer->Write();

    p4est_destroy(p4est);
    p4est_connectivity_destroy(conn);

    return 0;
}
