//                                MFEM Example 6
//
// Compile with: make ex6
//
// Sample runs:  ex6 -m ../data/square-disc.mesh -o 1
//               ex6 -m ../data/square-disc.mesh -o 2
//               ex6 -m ../data/square-disc-nurbs.mesh -o 2
//               ex6 -m ../data/star.mesh -o 3
//               ex6 -m ../data/escher.mesh -o 1
//               ex6 -m ../data/fichera.mesh -o 2
//               ex6 -m ../data/disc-nurbs.mesh -o 2
//               ex6 -m ../data/ball-nurbs.mesh
//               ex6 -m ../data/pipe-nurbs.mesh
//               ex6 -m ../data/star-surf.mesh -o 2
//               ex6 -m ../data/square-disc-surf.mesh -o 2
//               ex6 -m ../data/amr-quad.mesh
//
// Description:  This is a version of Example 1 with a simple adaptive mesh
//               refinement loop. The problem being solved is again the Laplace
//               equation -Delta u = 1 with homogeneous Dirichlet boundary
//               conditions. The problem is solved on a sequence of meshes which
//               are locally refined in a conforming (triangles, tetrahedrons)
//               or non-conforming (quadrilateral, hexahedrons) manner according
//               to a simple ZZ error estimator.
//
//               The example demonstrates MFEM's capability to work with both
//               conforming and nonconforming refinements, in 2D and 3D, on
//               linear, curved and surface meshes. Interpolation of functions
//               from coarse to fine meshes, as well as persistent GLVis
//               visualization are also illustrated.
//
//               We recommend viewing Example 1 before viewing this example.

#include "mfem.hpp"
#include <fstream>
#include <iostream>

using namespace std;
using namespace mfem;

int main(int argc, char *argv[])
{
   // 1. Parse command-line options.
   const char *mesh_file = "../data/star.mesh";
   int order = 1;
   bool visualization = 1;

   OptionsParser args(argc, argv);
   args.AddOption(&mesh_file, "-m", "--mesh",
                  "Mesh file to use.");
   args.AddOption(&order, "-o", "--order",
                  "Finite element order (polynomial degree).");
   args.AddOption(&visualization, "-vis", "--visualization", "-no-vis",
                  "--no-visualization",
                  "Enable or disable GLVis visualization.");
   args.Parse();
   if (!args.Good())
   {
      args.PrintUsage(cout);
      return 1;
   }
   args.PrintOptions(cout);

   // 2. Read the mesh from the given mesh file. We can handle triangular,
   //    quadrilateral, tetrahedral, hexahedral, surface and volume meshes with
   //    the same code.
   Mesh mesh(mesh_file, 1, 1);
   int dim = mesh.Dimension();
   int sdim = mesh.SpaceDimension();

   // 3. Since a NURBS mesh can currently only be refined uniformly, we need to
   //    convert it to a piecewise-polynomial curved mesh. First we refine the
   //    NURBS mesh a bit more and then project the curvature to quadratic Nodes.
   if (mesh.NURBSext)
   {
      for (int i = 0; i < 2; i++)
      {
         mesh.UniformRefinement();
      }
      mesh.SetCurvature(2);
   }

   // 4. Define a finite element space on the mesh. The polynomial order is
   //    one (linear) by default, but this can be changed on the command line.
   H1_FECollection fec(order, dim);
   FiniteElementSpace fespace(&mesh, &fec);

   // 5. As in Example 1, we set up bilinear and linear forms corresponding to
   //    the Laplace problem -\Delta u = 1. We don't assemble the discrete
   //    problem yet, this will be done in the main loop.
   BilinearForm a(&fespace);
   LinearForm b(&fespace);

   ConstantCoefficient one(1.0);
   ConstantCoefficient zero(0.0);

   a.AddDomainIntegrator(new DiffusionIntegrator(one));
   b.AddDomainIntegrator(new DomainLFIntegrator(one));

   // 6. The solution vector x and the associated finite element grid function
   //    will be maintained over the AMR iterations. We initialize it to zero.
   GridFunction x(&fespace);
   x = 0;

   // 7. All boundary attributes will be used for essential (Dirichlet) BC.
   MFEM_VERIFY(mesh.bdr_attributes.Size() > 0,
               "Boundary attributes required in the mesh.");
   Array<int> ess_bdr(mesh.bdr_attributes.Max());
   ess_bdr = 1;

   // 8. Connect to GLVis.
   char vishost[] = "localhost";
   int  visport   = 19916;
   socketstream sol_sock;
   if (visualization)
   {
      sol_sock.open(vishost, visport);
   }

   // 9. The main AMR loop. In each iteration we solve the problem on the
   //    current mesh, visualize the solution, estimate the error on all
   //    elements, refine the worst elements and update all objects to work
   //    with the new mesh.
   const int max_dofs = 50000;
   for (int it = 0; ; it++)
   {
      int cdofs = fespace.GetTrueVSize();
      cout << "\nAMR iteration " << it << endl;
      cout << "Number of unknowns: " << cdofs << endl;

      // 10. Assemble the stiffness matrix and the right-hand side. Note that
      //     MFEM doesn't care at this point if the mesh is nonconforming (i.e.,
      //     contains hanging nodes). The FE space is considered 'cut' along
      //     hanging edges/faces.
      a.Assemble();
      b.Assemble();

      // 11. Set Dirichlet boundary values in the GridFunction x.
      //     Determine the list of Dirichlet true DOFs in the linear system.
      Array<int> ess_tdof_list;
      x.ProjectBdrCoefficient(zero, ess_bdr);
      fespace.GetEssentialTrueDofs(ess_bdr, ess_tdof_list);

      // 12. Create the linear system: eliminate boundary conditions, constrain
      //     hanging nodes and possibly apply other transformations. The system
      //     will be solved for true (unconstrained) DOFs only.
      SparseMatrix A;
      Vector B, X;
      const int copy_interior = 1;
      a.FormLinearSystem(ess_tdof_list, x, b, A, X, B, copy_interior);

#ifndef MFEM_USE_SUITESPARSE
      // 13. Define a simple symmetric Gauss-Seidel preconditioner and use it to
      //     solve the linear system with PCG.
      GSSmoother M(A);
      PCG(A, M, B, X, 3, 200, 1e-12, 0.0);
#else
      // 13. If MFEM was compiled with SuiteSparse, use UMFPACK to solve the
      //     the linear system.
      UMFPackSolver umf_solver;
      umf_solver.Control[UMFPACK_ORDERING] = UMFPACK_ORDERING_METIS;
      umf_solver.SetOperator(A);
      umf_solver.Mult(B, X);
#endif

      // 14. After solving the linear system, reconstruct the solution as a finite
      //     element grid function. Constrained nodes are interpolated from true
      //     DOFs (it may therefore happen that dim(x) >= dim(X)).
      a.RecoverFEMSolution(X, b, x);

      // 15. Send solution by socket to the GLVis server.
      if (visualization && sol_sock.good())
      {
         sol_sock.precision(8);
         sol_sock << "solution\n" << mesh << x << flush;
      }

      if (cdofs > max_dofs)
      {
         break;
      }

      // 16. Estimate element errors using the Zienkiewicz-Zhu error estimator.
      //     The bilinear form integrator must have the 'ComputeElementFlux'
      //     method defined.
      Vector errors(mesh.GetNE());
      Array<int> aniso_flags;
      {
         DiffusionIntegrator flux_integrator(one);
         FiniteElementSpace flux_fespace(&mesh, &fec, sdim);
         GridFunction flux(&flux_fespace);
         ZZErrorEstimator(flux_integrator, x, flux, errors, &aniso_flags);
      }

      // 17. Make a list of elements whose error is larger than a fraction of
      //     the maximum element error. These elements will be refined.
      Array<Refinement> ref_list;
      const double frac = 0.7;
      double threshold = frac * errors.Max();
      for (int i = 0; i < errors.Size(); i++)
      {
         if (errors[i] >= threshold)
         {
            ref_list.Append(Refinement(i, aniso_flags[i]));
         }
      }

      // 18. Refine the selected elements.
      mesh.GeneralRefinement(ref_list);

      // 19. Update the space to reflect the new state of the mesh. Also,
      //     interpolate the solution x so that it lies in the new space but
      //     represents the same function. This saves solver iterations since
      //     we'll have a good initial guess of x in the next step. Internally,
      //     FiniteElementSpace::Update() calculates an interpolation matrix
      //     which is then used by GridFunction::Update().
      fespace.Update();
      x.Update();

      // 20. Inform also the bilinear and linear forms that the space has
      //     changed.
      a.Update();
      b.Update();
   }

   return 0;
}
