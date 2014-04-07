#include "Andersen.h"

#include "llvm/ADT/Statistic.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/CommandLine.h"

using namespace llvm;

cl::opt<bool> DumpDebugInfo("dump-debug", cl::desc("Dump debug info into stderr"), cl::init(false), cl::Hidden);
cl::opt<bool> DumpResultInfo("dump-result", cl::desc("Dump result info into stderr"), cl::init(false), cl::Hidden);
cl::opt<bool> DumpConstraintInfo("dump-cons", cl::desc("Dump constraint info into stderr"), cl::init(false), cl::Hidden);

void Andersen::getAnalysisUsage(AnalysisUsage &AU) const
{
	AU.addRequired<DataLayoutPass>();
	AU.setPreservesAll();
}

bool Andersen::runOnModule(Module &M)
{
	dataLayout = &(getAnalysis<DataLayoutPass>().getDataLayout());
	nodeFactory.setDataLayout(dataLayout);

	collectConstraints(M);

	if (DumpDebugInfo)
		dumpConstraintsPlainVanilla();

	optimizeConstraints();

	if (DumpConstraintInfo)
		dumpConstraints();

	solveConstraints();

	if (DumpDebugInfo)
	{
		errs() << "\n";
		dumpPtsGraphPlainVanilla();
	}

	if (DumpResultInfo)
	{
		nodeFactory.dumpNodeInfo();
		errs() << "\n";
		dumpPtsGraphPlainVanilla();	
	}
	

	return false;
}

void Andersen::releaseMemory()
{
}

void Andersen::dumpConstraint(const AndersConstraint& item) const
{
	NodeIndex dest = item.getDest();
	NodeIndex src = item.getSrc();

	switch (item.getType())
	{
		case AndersConstraint::COPY:
		{
			nodeFactory.dumpNode(dest);
			errs() << " = ";
			nodeFactory.dumpNode(src);
			break;
		}
		case AndersConstraint::LOAD:
		{
			nodeFactory.dumpNode(dest);
			errs() << " = *";
			nodeFactory.dumpNode(src);
			break;
		}
		case AndersConstraint::STORE:
		{
			errs() << "*";
			nodeFactory.dumpNode(dest);
			errs() << " = ";
			nodeFactory.dumpNode(src);
			break;
		}
		case AndersConstraint::ADDR_OF:
		{
			nodeFactory.dumpNode(dest);
			errs() << " = &";
			nodeFactory.dumpNode(src);
		}
	}

	errs() << "\n";
}

void Andersen::dumpConstraints() const
{
	errs() << "\n----- Constraints -----\n";
	for (auto const& item: constraints)
		dumpConstraint(item);
	errs() << "----- End of Print -----\n";
}

void Andersen::dumpConstraintsPlainVanilla() const
{
	for (auto const& item: constraints)
	{
		errs() << item.getType() << " " << item.getDest() << " " << item.getSrc() << " 0\n";
	}
}

void Andersen::dumpPtsGraphPlainVanilla() const
{
	for (unsigned i = 0, e = nodeFactory.getNumNodes(); i < e; ++i)
	{
		NodeIndex rep = nodeFactory.getMergeTarget(i);
		auto ptsItr = ptsGraph.find(rep);
		if (ptsItr != ptsGraph.end())
		{
			errs() << i << " ";
			for (auto v: ptsItr->second)
				errs() << v << " ";
			errs() << "\n";
		}
	}
}

char Andersen::ID = 0;
static RegisterPass<Andersen> X("anders", "Andersen's inclusion-based points-to analysis", true, true);

