const std::array<std::string, numsolprint+1> solprinttxt {
    "0 Summary",
    "1 Report",
    "2 Quiet",
    "0 Off",
    "1 On",
    "2 Silent"
};

const std::array<std::string, numhandlestat+1> handlestattxt {
    "0 Unknown",
    "1 Running",
    "2 Ready",
    "3 Failure"
};

const std::array<std::string, numsolvelink+1> solvelinktxt {
    "0 Chain Script",
    "1 Call Script",
    "2 Call Module",
    "3 Async Grid",
    "4 Async Simulate",
    "5 Load Library",
    "6 ASync Threads",
    "7 Threads Simulate"
};

const std::array<std::string, numsolveopt+1> solveopttxt {
    "0 Replace",
    "1 Merge",
    "2 Clear"
};

const std::array<std::string, numsolm+1> solvestatustxt {
    "1 Normal Completion",
    "2 Iteration Interrupt",
    "3 Resource Interrupt",
    "4 Terminated By Solver",
    "5 Evaluation Interrupt",
    "6 Capability Problems",
    "7 Licensing Problems",
    "8 User Interrupt",
    "9 Setup Failure",
    "10 Solver Failure",
    "11 Internal Solver Failure",
    "12 Solve Processing Skipped",
    "13 System Failure"
};

const std::array<std::string, nummodm+1> modelstatustxt {
    "1 Optimal",
    "2 Locally Optimal",
    "3 Unbounded",
    "4 Infeasible",
    "5 Locally Infeasible",
    "6 Intermediate Infeasible",
    "7 Feasible Solution",
    "8 Integer Solution",
    "9 Intermediate Non-Integer",
    "10 Integer Infeasible",
    "11 Licensing Problem",
    "12 Error Unknown",
    "13 Error No Solution",
    "14 No Solution Returned",
    "15 Solved Unique",
    "16 Solved",
    "17 Solved Singular",
    "18 Unbounded - No Solution",
    "19 Infeasible - No Solution"
};

const std::array<std::string, numplatformcode+1> platformcodetxt {
    "0 Unknown",
    "1 DEG",
    "2 LEG",
    "3 WEX"
};
