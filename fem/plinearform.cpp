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

#ifdef MFEM_USE_MPI

#include "fem.hpp"

void ParLinearForm::Update(ParFiniteElementSpace *pf)
{
   if (pf) pfes = pf;

   LinearForm::Update(pfes);
}

void ParLinearForm::Update(ParFiniteElementSpace *pf, Vector &v, int v_offset)
{
	if(pf) pfes = pf;

	LinearForm::Update(pfes, v, v_offset);
}

void ParLinearForm::ParallelAssemble(Vector &tv)
{
   pfes->Dof_TrueDof_Matrix()->MultTranspose(*this, tv);
}

HypreParVector *ParLinearForm::ParallelAssemble()
{
   HypreParVector *tv = pfes->NewTrueDofVector();
   pfes->Dof_TrueDof_Matrix()->MultTranspose(*this, *tv);
   return tv;
}

#endif
