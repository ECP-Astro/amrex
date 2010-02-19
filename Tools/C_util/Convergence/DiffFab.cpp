
//
// $Id: DiffFab.cpp,v 1.8 2010-02-19 23:04:42 ajnonaka Exp $
//

#include <new>
#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <string>
using std::ios;
using std::set_new_handler;

#include <unistd.h>

#include "REAL.H"
#include "Box.H"
#include "FArrayBox.H"
#include "ParmParse.H"
#include "Utility.H"
#include "VisMF.H"

#ifndef NDEBUG
#include "TV_TempWrite.H"
#endif

#include "AVGDOWN_F.H"

#define GARBAGE 666.e+40

static
void
PrintUsage (const char* progName)
{
    std::cout << '\n';
    std::cout << "Usage:" << '\n';
    std::cout << progName << '\n';
    std::cout << "    infile  = inputFileName" << '\n';
    std::cout << "    exact   = exactFileName" << '\n';
    std::cout << "    outfile = outputFileName" << '\n';
    std::cout << "   [-help]" << '\n';
    std::cout << '\n';
    exit(1);
}

IntVect
getRefRatio(const Box& crse,
	    const Box& fine)
{
    // Compute refinement ratio between crse and fine boxes, return invalid
    // IntVect if there is none suitable
    IntVect ref_ratio;
    for (int i=0; i<BL_SPACEDIM; ++i)
	ref_ratio[i] = fine.length(i) / crse.length(i);

    // Check results
    Box test1 = ::Box(fine).coarsen(ref_ratio);
    Box test2 = ::Box(test1).refine(ref_ratio);
    if (test1 != crse  ||  test2 != fine)
	ref_ratio = IntVect();
    return ref_ratio;
}

int
main (int   argc,
      char* argv[])
{
    if (argc == 1)
        PrintUsage(argv[0]);

    BoxLib::Initialize(argc,argv);

    ParmParse pp;

    FArrayBox::setFormat(FABio::FAB_IEEE_32);
    //
    // Scan the arguments.
    //
    std::string iFileDir, iFile, eFile, oFile, oFileDir;


    pp.query("infile", iFile);
    if (iFile.empty())
        BoxLib::Abort("You must specify `infile'");

    pp.query("exact", eFile);
    if (eFile.empty())
        BoxLib::Abort("You must specify `exact' file");

    pp.query("outfile", oFile);
    if (oFile.empty())
        BoxLib::Abort("You must specify `outfile'");

    std::ifstream is1(iFile.c_str(),ios::in);
    std::ifstream is2(eFile.c_str(),ios::in);
    std::ofstream os (oFile.c_str(),ios::out);

    FArrayBox dataI, dataE;
    dataI.readFrom(is1);
    dataE.readFrom(is2);

    BL_ASSERT(dataI.nComp() == dataE.nComp());
	
    //
    // Compute the error
    //
    int nComp = dataI.nComp();
    const Box& domainI = dataI.box();
    const Box& domainE = dataE.box();
    IntVect refine_ratio = getRefRatio(domainI, domainE);

    if (refine_ratio == IntVect())
      BoxLib::Error("Cannot find refinement ratio from data to exact");
    
    FArrayBox error(domainI,nComp);
    error.setVal(GARBAGE);
 
    FArrayBox exactAvg(domainI,nComp);
      
    FORT_CV_AVGDOWN(exactAvg.dataPtr(),
		    ARLIM(exactAvg.loVect()), 
		    ARLIM(exactAvg.hiVect()), &nComp,
		    dataE.dataPtr(),
		    ARLIM(dataE.loVect()), ARLIM(dataE.hiVect()),
		    domainI.loVect(), domainI.hiVect(),
		    refine_ratio.getVect());

    error.copy(exactAvg);
    error.minus(dataI);
    error.writeOn(os);

    Box bx = error.box();
    int points = bx.numPts();

    std::cout << "L0 NORM                           = " << error.norm(0) 
	      << std::endl;
    std::cout << "L1 NORM (normalized by 1/N)       = " << error.norm(1)/points
	      << std::endl;
    std::cout << "L2 NORM (normalized by 1/sqrt(N)) = " << error.norm(2)/sqrt(points) 
	      << std::endl;

    BoxLib::Finalize();

}
