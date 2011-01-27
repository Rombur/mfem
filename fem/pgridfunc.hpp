// Copyright (c) 2010, Lawrence Livermore National Security, LLC. Produced at
// the Lawrence Livermore National Laboratory. LLNL-CODE-443211. All Rights
// reserved. See file COPYRIGHT for details.
//
// This file is part of the MFEM library. For more information and source code
// availability see http://mfem.googlecode.com.
//
// MFEM is free software; you can redistribute it and/or modify it under the
// terms of the GNU Lesser General Public License (as published by the Free
// Software Foundation) version 2.1 dated February 1999.

#ifndef MFEM_PGRIDFUNC
#define MFEM_PGRIDFUNC

/// Class for parallel grid function
class ParGridFunction : public GridFunction
{
protected:
   ParFiniteElementSpace *pfes;

public:
   ParGridFunction(ParFiniteElementSpace *pf) : GridFunction(pf), pfes(pf) { }

   /// Construct a ParGridFunction corresponding to *pf and the data from *gf
   ParGridFunction(ParFiniteElementSpace *pf, GridFunction *gf);

   /** Creates grid function on (all) dofs from a given vector on the true dofs,
       i.e. P tv. */
   ParGridFunction(ParFiniteElementSpace *pf, HypreParVector *tv);

   /// Returns the vector averaged on the true dofs.
   HypreParVector *ParallelAvarage();

   double ComputeL2Error(Coefficient *exsol[],
                         const IntegrationRule *irs[] = NULL) const;

   /// Merge the local grid functions
   void SaveAsOne(ostream &out = cout);
};

#endif
