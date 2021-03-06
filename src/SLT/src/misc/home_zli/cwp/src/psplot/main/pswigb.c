/* Copyright (c) Colorado School of Mines, 1990.
/* All rights reserved.                       */

char *sdoc =
"PSWIGB - PostScript WIGgle-trace plot of f(x1,x2) via Bitmap\n"
"Best for many traces.  Use PSWIGP (Polygon version) for few traces.\n"
"\n"
"pswigb n1= [optional parameters] <binaryfile >postscriptfile\n"
"\n"
"Required Parameters:\n"
"n1                     number of samples in 1st (fast) dimension\n"
"\n"
"Optional Parameters:\n"
"d1=1.0                 sampling interval in 1st dimension\n"
"f1=d1                  first sample in 1st dimension\n"
"n2=all                 number of samples in 2nd (slow) dimension\n"
"d2=1.0                 sampling interval in 2nd dimension\n"
"f2=d2                  first sample in 2nd dimension\n"
"x2=f2,f2+d2,...        array of sampled values in 2nd dimension\n"
"bias=0.0               data value corresponding to location along axis 2\n"
"perc=100.0             percentile for determining clip\n"
"clip=(perc percentile) data values < bias+clip and > bias-clip are clipped\n"
"xcur=1.0               wiggle excursion in traces corresponding to clip\n"
"wt=1                   =0 for no wiggle-trace; =1 for wiggle-trace\n"
"va=1                   =0 for no variable-area; =1 for variable-area fill\n"
"nbpi=72                number of bits per inch at which to rasterize\n"
"verbose=1              =1 for info printed on stderr (0 for no info)\n"
"xbox=1.5               offset in inches of left side of axes box\n"
"ybox=1.5               offset in inches of bottom side of axes box\n"
"wbox=6.0               width in inches of axes box\n"
"hbox=8.0               height in inches of axes box\n"
"x1beg=x1min            value at which axis 1 begins\n"
"x1end=x1max            value at which axis 1 ends\n"
"d1num=0.0              numbered tic interval on axis 1 (0.0 for automatic)\n"
"f1num=x1min            first numbered tic on axis 1 (used if d1num not 0.0)\n"
"n1tic=1                number of tics per numbered tic on axis 1\n"
"grid1=none             grid lines on axis 1 - none, dot, dash, or solid\n"
"label1=                label on axis 1\n"
"x2beg=x2min            value at which axis 2 begins\n"
"x2end=x2max            value at which axis 2 ends\n"
"d2num=0.0              numbered tic interval on axis 2 (0.0 for automatic)\n"
"f2num=x2min            first numbered tic on axis 2 (used if d2num not 0.0)\n"
"n2tic=1                number of tics per numbered tic on axis 2\n"
"grid2=none             grid lines on axis 2 - none, dot, dash, or solid\n"
"label2=                label on axis 2\n"
"labelfont=Helvetica    font name for axes labels\n"
"labelsize=12           font size for axes labels\n"
"title=                 title of plot\n"
"titlefont=Helvetica-Bold font name for title\n"
"titlesize=24           font size for title\n"
"style=seismic          normal (axis 1 horizontal, axis 2 vertical) or\n"
"                       seismic (axis 1 vertical, axis 2 horizontal)\n"
"\n"
"AUTHOR:  Dave Hale, Colorado School of Mines, 05/29/90\n"
"\n";

#include "par.h"
#include "psplot.h"

main (argc,argv)
int argc; char **argv;
{
	int n1,n2,nbpi,n1tic,n2tic,nfloats,wt,va,bbox[4],
		i1,i2,npar,grid1,grid2,style,
		n1bits,n2bits,nbpr,i1beg,i1end,if1r,n1r,
		b1fz,b1lz,n2in,nz,iz,i,nbytes,verbose;
	float labelsize,titlesize,bias,perc,clip,xcur,
		d1,f1,d2,f2,*x2,*z,zmin,zmax,
		xbox,ybox,wbox,hbox,
		x1beg,x1end,x2beg,x2end,
		x1min,x1max,x2min,x2max,
		d1num,f1num,d2num,f2num,
		*temp,p2beg,p2end,matrix[6],
		bscale,boffset,bxcur,bx2;
	unsigned char *bits;
	char *label1="",*label2="",*title="",
		*labelfont="Helvetica",*titlefont="Helvetica-Bold",
		*styles="seismic",*grid1s="none",*grid2s="none";
	FILE *infp=stdin;

	/* initialize getpar */
	initargs(argc,argv);
	askdoc(1);

	/* get parameters describing 1st dimension sampling */
	if (!getparint("n1",&n1))
		err("Must specify number of samples in 1st dimension!\n");
	d1 = 1.0;  getparfloat("d1",&d1);
	f1 = d1;  getparfloat("f1",&f1);
	x1min = (d1>0.0)?f1:f1+(n1-1)*d1;
	x1max = (d1<0.0)?f1:f1+(n1-1)*d1;

	/* get parameters describing 2nd dimension sampling */
	if ((n2=countparval("x2"))==0 && !getparint("n2",&n2)) {
			if (fseek(infp,0L,2)!=0)
				err("must specify n2 if in a pipe!\n");
			nfloats = eftell(infp)/sizeof(float);
			efseek(infp,0L,0);
			n2 = nfloats/n1;
	}
	x2 = ealloc1float(n2);
	if (!getparfloat("x2",x2)) {
		d2 = 1.0;  getparfloat("d2",&d2);
		f2 = d2;  getparfloat("f2",&f2);
		for (i2=0; i2<n2; i2++)
			x2[i2] = f2+i2*d2;
	}
	for (i2=1,x2min=x2max=x2[0]; i2<n2; i2++) {
		x2min = MIN(x2min,x2[i2]);
		x2max = MAX(x2max,x2[i2]);
	}

	/* read binary data to be plotted */
	nz = n1*n2;
	z = ealloc1float(nz);
	if (fread(z,sizeof(float),nz,infp)!=nz)
		err("error reading input file!\n");

	/* if necessary, subtract bias */
	if (getparfloat("bias",&bias) && bias!=0.0)
		for (iz=0; iz<nz; iz++)
			z[iz] -= bias;
	
	/* if necessary, determine clip from percentile */
	if (!getparfloat("clip",&clip)) {
		perc = 100.0;  getparfloat("perc",&perc);
		temp = ealloc1float(nz);
		for (iz=0; iz<nz; iz++)
			temp[iz] = fabs(z[iz]);
		iz = (nz*perc/100.0);
		if (iz<0) iz = 0;
		if (iz>nz-1) iz = nz-1;
		qkfind(iz,nz,temp);
		clip = temp[iz];
		free1float(temp);
	}
	verbose = 1;  getparint("verbose",&verbose);
	if (verbose) warn("clip=%g\n",clip);

	/* get wiggle-trace-variable-area parameters */
	wt = 1;  getparint("wt",&wt);
	va = 1;  getparint("va",&va);

	/* get axes parameters */
	xbox = 1.5; getparfloat("xbox",&xbox);
	ybox = 1.5; getparfloat("ybox",&ybox);
	wbox = 6.0; getparfloat("wbox",&wbox);
	hbox = 8.0; getparfloat("hbox",&hbox);
	x1beg = x1min; getparfloat("x1beg",&x1beg);
	x1end = x1max; getparfloat("x1end",&x1end);
	d1num = 0.0; getparfloat("d1num",&d1num);
	f1num = x1min; getparfloat("f1num",&f1num);
	n1tic = 1; getparint("n1tic",&n1tic);
	getparstring("grid1",&grid1s);
	if (STREQ("dot",grid1s)) grid1 = DOT;
	else if (STREQ("dash",grid1s)) grid1 = DASH;
	else if (STREQ("solid",grid1s)) grid1 = SOLID;
	else grid1 = NONE;
	getparstring("label1",&label1);
	x2beg = x2min; getparfloat("x2beg",&x2beg);
	x2end = x2max; getparfloat("x2end",&x2end);
	d2num = 0.0; getparfloat("d2num",&d2num);
	f2num = 0.0; getparfloat("f2num",&f2num);
	n2tic = 1; getparint("n2tic",&n2tic);
	getparstring("grid2",&grid2s);
	if (STREQ("dot",grid2s)) grid2 = DOT;
	else if (STREQ("dash",grid2s)) grid2 = DASH;
	else if (STREQ("solid",grid2s)) grid2 = SOLID;
	else grid2 = NONE;
	getparstring("label2",&label2);
	getparstring("labelfont",&labelfont);
	labelsize = 18.0; getparfloat("labelsize",&labelsize);
	getparstring("title",&title);
	getparstring("titlefont",&titlefont);
	titlesize = 24.0; getparfloat("titlesize",&titlesize);
	getparstring("style",&styles);
	if (STREQ("normal",styles)) style = NORMAL;
	else style = SEISMIC;

	/* determine bitmap dimensions and allocate space for bitmap */
	nbpi = 72;  getparint("nbpi",&nbpi);
	n1bits = nbpi*((style==NORMAL)?wbox:hbox);
	n2bits = nbpi*((style==NORMAL)?hbox:wbox);
	nbpr = 1+(n2bits-1)/8;
	bits = ealloc1(nbpr*n1bits,sizeof(unsigned char));
	for (i=0,nbytes=nbpr*n1bits; i<nbytes; i++)
		bits[i] = 0;

	/* determine number of traces that fall within axis 2 bounds */
	x2min = MIN(x2beg,x2end);
	x2max = MAX(x2beg,x2end);
	for (i2=0,n2in=0; i2<n2; i2++)
		if (x2[i2]>=x2min && x2[i2]<=x2max) n2in++;

	/* determine pads for wiggle excursion along axis 2 */
	xcur = 1.0;  getparfloat("xcur",&xcur);
	xcur = fabs(xcur);
	if (n2in>1) xcur *= (x2max-x2min)/(n2in-1);
	p2beg = (x2end>x2beg)?-xcur:xcur;
	p2end = (x2end>x2beg)?xcur:-xcur;

	/* determine scale and offset to map x2 units to bitmap units */
	bscale = (n2bits-1)/(x2end+p2end-x2beg-p2beg);
	boffset = -(x2beg+p2beg)*bscale;
	bxcur = xcur*bscale;

	/* adjust x1beg and x1end to fall on sampled values */
	i1beg = NINT((x1beg-f1)/d1);
	i1beg = MAX(0,MIN(n1-1,i1beg));
	x1beg = f1+i1beg*d1;
	i1end = NINT((x1end-f1)/d1);
	i1end = MAX(0,MIN(n1-1,i1end));
	x1end = f1+i1end*d1;

	/* determine first sample and number of samples to rasterize */
	if1r = MIN(i1beg,i1end);
	n1r = MAX(i1beg,i1end)-if1r+1;

	/* determine bits corresponding to first and last samples */
	b1fz = (x1end>x1beg)?0:n1bits-1;
	b1lz = (x1end>x1beg)?n1bits-1:0;

	/* rasterize traces */
	for (i2=0; i2<n2; i2++,z+=n1) {

		/* skip traces not in bounds */
		if (x2[i2]<x2min || x2[i2]>x2max) continue;

		/* determine bitmap coordinate of trace */
		bx2 = boffset+x2[i2]*bscale;

		/* rasterize one trace */
		rfwtva(n1r,&z[if1r],-clip,clip,va?0:clip,
			(int)(bx2-bxcur),(int)(bx2+bxcur),b1fz,b1lz,
			wt,nbpr,bits);
	}

	/* invert bitmap for PostScript (for which black=0, white=1) */
	for (i=0,nbytes=nbpr*n1bits; i<nbytes; i++)
		bits[i] = ~bits[i];

	/* begin PostScript */
	beginps();
	newpage("1",1);

	/* convert axes box parameters from inches to points */
	xbox *= 72.0;
	ybox *= 72.0;
	wbox *= 72.0;
	hbox *= 72.0;

	/* save graphics state */
	gsave();

	/* translate coordinate system by box offset */
	translate(xbox,ybox);

	/* determine image matrix */
	if (style==NORMAL) {
		matrix[0] = 0;  matrix[1] = n1bits;  matrix[2] = n2bits;
		matrix[3] = 0;  matrix[4] = 0;  matrix[5] = 0;
	} else {
		matrix[0] = n2bits;  matrix[1] = 0;  matrix[2] = 0;
		matrix[3] = -n1bits;  matrix[4] = 0;  matrix[5] = n1bits;
	}

	scale(wbox,hbox);

	/* draw the image (before axes so grid lines are visible) */
	image(n2bits,n1bits,1,matrix,bits);

	/* restore graphics state */
	grestore();

	/* set bounding box */
	psAxesBBox(
		xbox,ybox,wbox,hbox,
		labelfont,labelsize,
		titlefont,titlesize,
		style,bbox);
	boundingbox(bbox[0],bbox[1],bbox[2],bbox[3]);

	/* draw axes and title */
	psAxesBox(
		xbox,ybox,wbox,hbox,
		x1beg,x1end,0.0,0.0,
		d1num,f1num,n1tic,grid1,label1,
		x2beg,x2end,p2beg,p2end,
		d2num,f2num,n2tic,grid2,label2,
		labelfont,labelsize,
		title,titlefont,titlesize,
		style);

	/* end PostScript */
	showpage();
	endps();
}
