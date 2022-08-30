#pragma once
#include "../global/gmsspecs.h"

// Interface
namespace gdlib::gmsglob {
	enum tssymbol {
		ssyeq, ssygt, ssyge, ssylt, ssyle,
		ssyne,
		ssyplus, ssysubtr,
		ssymult, ssydiv,
		ssylagpp, ssylagmm,
		ssyasspar, ssyassequ, ssyassdol,
		ssyor, ssyxor,
		ssyno, ssyyes, ssyna, ssyinf, ssyeps,
		ssysum, ssyprod, ssysmin, ssysmax,
		ssysca, ssyacr, ssymod, ssyset,
		ssypar, ssyvar, ssyequ, ssyfile,
		ssypro, ssypre, ssymac, ssyfunc,
		ssyendloop, ssyendif, ssyendwhile, ssyendfor,
		ssyfre, ssybin, ssypos, ssyneg, ssyint,
		ssysos1, ssysos2, ssysemi, ssysemiint,
		ssymin, ssymax,
		// !!! Be very careful introducing new symbols before
        // !!! ssyeque. On different spots we assume to see 53
        // !!! elements before that -> better do it further down
        // !!! the list; compare e.g.
        // !!! EQU_USERINFO_BASE in gmsspecs.pas
		ssyeque, ssyequg, ssyequl, ssyequn, ssyequx,
		ssyequc, ssyequb, ssysetm, ssysets,
		/* there is an ord in init.inc goe into symtab  */
		ssydisp, ssyabort, ssyexec, ssyload, ssyunload,
		ssyloadpoint, ssyloadhandle, ssyloaddc, ssyunloaddi, ssyunloadidx,
		ssyBreak, ssyContinue, ssysand , ssysor,
		ssyput, ssyptl, ssyphd, ssypclear,
		ssyppg, ssypcl,
		ssyround, ssysquare, ssycurly, ssyimp, ssyeqv,
		ssypbruce, ssyundf,
		ssyother, numtssymbols
	};

	const std::array ssymboltext = {
			"eq", "gt", "ge", "lt", "le",
			"ne",
			"plus", "subtr",
			"mult", "div",
			"lagpp", "lagmm",
			"asspar", "assequ", "assdol",
			"or", "xor",
			"no", "yes", "na", "inf", "eps",
			"sum", "prod", "smin", "smax",
			"sca", "acr", "mod", "set",
			"par", "var", "equ", "file",
			"pro", "pre", "mac", "func",
			"endloop", "endif", "endwhile", "endfor",
			"fre", "bin", "pos", "neg", "int",
			"sos1", "sos2", "semi", "semiint",
			"min", "max",
			"eque", "equg", "equl", "equn", "equx",
			"equc", "equb", "set", "singleton",
			/* there is an ord in init.inc goe into symtab  */
			"disp", "abort", "exec", "load", "unload",
			"loadpoint", "loadhandle", "loaddc", "unloaddi", "unloadidx",
			"break", "continue", "sand", "sor",
			"put", "ptl", "phd", "pclear",
			"ppg", "pcl",
			"round", "square", "curly", "imp", "eqv",
			"pbruce", "undf",
			"other"
	};

	// default values for variables and equations
	extern std::array<global::gmsspecs::tvarreca, global::gmsspecs::stypsemiint+1> defrecvar;
	extern std::array<global::gmsspecs::tvarreca, ssyequb+1> defrecequ;

	void InitDefaultRecords();

}