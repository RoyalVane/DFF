#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <mex.h>
#include "Matrix.hh"
#include "csa.hh"
#include "match_mrf.hh"

extern "C" {

static const double sigmaXDefault = 1;
static const double sigmaYDefault = 3;
static const double maxSpatialCostDefault = 4.5;//(d=3*sigma)
static const double maxLoss = 128;
static const double neighSizeDefault = 16;
static const double wPairwiseDefault = 0.02;
static const double outlierCostDefault = 100*(maxLoss + maxSpatialCostDefault + wPairwiseDefault*(2*neighSizeDefault+1)*(2*neighSizeDefault+1));

void 
mexFunction (
    int nlhs, mxArray* plhs[],
    int nrhs, const mxArray* prhs[]) {

    // check number of arguments
    if (nlhs < 2) {
        mexErrMsgTxt("Too few output arguments.");
    }
    if (nlhs > 4) {
        mexErrMsgTxt("Too many output arguments.");
    }
    if (nrhs < 5) {
        mexErrMsgTxt("Too few input arguments.");
    }
    if (nrhs > 11) {
        mexErrMsgTxt("Too many input arguments.");
    }

    // get arguments
    double* pMaskMap = mxGetPr(prhs[0]); // valid areas for edge matching
    double* pGtMap = mxGetPr(prhs[1]); // ground truth edge
    double* pEdgeProb = mxGetPr(prhs[2]);
    double* pBdryTan = mxGetPr(prhs[3]);
    double* pPreMatch = mxGetPr(prhs[4]); // previous match result for neighborhood pairwise cost
    const double sigmaX = (nrhs>5) ? mxGetScalar(prhs[5]) : sigmaXDefault;
    const double sigmaY = (nrhs>6) ? mxGetScalar(prhs[6]) : sigmaYDefault;
    const double maxSpatialCost = (nrhs>7) ? mxGetScalar(prhs[7]) : maxSpatialCostDefault;
    const double outlierCost = (nrhs>8) ? mxGetScalar(prhs[8]) : outlierCostDefault;
    const double neighSize = (nrhs>9) ? mxGetScalar(prhs[9]) : neighSizeDefault;
    const double wPairwise = (nrhs>10) ? mxGetScalar(prhs[10]) : wPairwiseDefault;

    // check arguments
    if (mxGetM(prhs[0]) != mxGetM(prhs[1]) || mxGetN(prhs[0]) != mxGetN(prhs[1])) {
        mexErrMsgTxt("candMap and gtMap must have the same size");
    }
    if (mxGetM(prhs[0]) != mxGetM(prhs[2]) || mxGetN(prhs[0]) != mxGetN(prhs[2])) {
        mexErrMsgTxt("candMap and edgeProb must have the same size");
    }
    if (mxGetM(prhs[0]) != mxGetM(prhs[3]) || mxGetN(prhs[0]) != mxGetN(prhs[3])) {
        mexErrMsgTxt("candMap and bdryTan must have the same size");
    }
    if (mxGetM(prhs[0]) != mxGetM(prhs[4]) || mxGetN(prhs[0]) != mxGetN(prhs[4])) {
        mexErrMsgTxt("candMap and pMatch must have the same size");
    }
    if (sigmaX <= 0) {
        mexErrMsgTxt("sigmaX must be > 0");
    }
    if (sigmaY <= 0) {
        mexErrMsgTxt("sigmaY must be > 0");
    }
    if (maxSpatialCost < 1) {
        mexErrMsgTxt("maxSpatialCost must be >= 1");
    }
    if (outlierCost <= 0) {
        mexErrMsgTxt("outlierCost must be > 0");
    }
    if (neighSize < 1) {
        mexErrMsgTxt("neighSize must be >= 1");
    }
    if (wPairwise < 0) {
        mexErrMsgTxt("neighSize must be >= 0");
    }

    // compute cost of boundary probability
    const int rows = mxGetM(prhs[0]);
    const int cols = mxGetN(prhs[0]);
    const double lossBound = maxLoss/2;
    mxArray *edgeCost = mxCreateDoubleMatrix(rows, cols, mxREAL);
    double *pEdgeCost = mxGetPr(edgeCost);
    for (int i=0; i<rows*cols; i++) {
        const double prob = *(pEdgeProb+i);
        assert(prob >= 0 && prob <= 1);
        *(pEdgeCost+i) = fmax(fmin(log(1-prob) - log(prob), lossBound), -lossBound) + lossBound;// Make sure loss is in [0, maxLoss], a safety measure for numerical stability.
       assert(*(pEdgeCost+i)<=maxLoss);
    }

    Matrix m1, m2;
    const double cost = solveAssignMRF(Matrix(rows, cols, pMaskMap), Matrix(rows, cols, pGtMap), Matrix(rows, cols, pEdgeCost), 
                                       Matrix(rows, cols, pBdryTan), Matrix(rows, cols, pPreMatch),
                                       sigmaX, sigmaY, maxSpatialCost, outlierCost, neighSize, wPairwise, m1, m2);// Modified by Zhiding
    mxDestroyArray(edgeCost);
    
    // set output arguments
    plhs[0] = mxCreateDoubleMatrix(rows, cols, mxREAL);
    plhs[1] = mxCreateDoubleMatrix(rows, cols, mxREAL);
    double* match1 = mxGetPr(plhs[0]);
    double* match2 = mxGetPr(plhs[1]);
    memcpy(match1,m1.data(),m1.numel()*sizeof(double));
    memcpy(match2,m2.data(),m2.numel()*sizeof(double));
    if (nlhs > 2) { plhs[2] = mxCreateDoubleScalar(cost); }
    if (nlhs > 3) { plhs[3] = mxCreateDoubleScalar(outlierCost); }
}

}; // extern "C"
