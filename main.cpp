/*
Grecode is a tool to modify gcode for CNC machines.
Copyright (C) 2010 Bernhard Kubicek

This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 3 of the License, or any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program; if not, see <http://www.gnu.org/licenses/>.
*/

#include "gdecoder.h"
#include <iostream>
#include <sstream>
#include <math.h>
#include <stdlib.h>
#include <vector>

using namespace std;
enum operations{none,xflip, yflip, xyexchange, cw,ccw,rot,shift,scale,knive,align,killN,parameterize,overlay,makeabsolut,copies,comment,zxtilt,zytilt,alength};
enum aligns{align_min,align_max,align_middle,  align_cmin,align_cmax,align_cmiddle, align_keep};  // c: with bounding box of the cuts only, otherwise also rapid moves



/// a 2d-rotation matrix and a shift. Will be used in future implementations, currently matrizes are set but unused 
void setmatrix(double *m,double *shift,  double m11,double m12,double m21,double m22, double sx,double sy)
{
	m[0]=m11;m[1]=m12;
	m[2]=m21;m[3]=m22;
	shift[0]=sx;
	shift[1]=sy;
}

//sorry for using global variables. they are the interaction between the main() routine and processParameters()
string outputfile="",inputfile="",outputfilegnuplot="";
operations op=none;
double rotangle=0,scalefactor=1,knivedelay=0;
double rotxangle=0,rotyangle=0;
double s[2];
double m[4];
aligns alignx,aligny;
int parm_minoccurence,parm_startnr;

int nrcopy[2];
double copyshift[2];
bool copiesRespect=false;
bool copiesRespectDistance=0;
char commenttype;
string commenttext;

struct overlaypoints //stores 4 coordinates-pairs
{
  //from
  float ax,ay;
  float bx,by;
  //to
  float nax,nay;
  float nbx,nby;

};

struct overlaypoints opoints;

vector<operations> ops;  //stores the chain of operations

void processParameters(int argc, char** argv)
{
  for(int i=1;i<argc;i++)
	{
		if(string(argv[i])=="-xflip")
		{
			ops.push_back(xflip);
			setmatrix(m,s, -1,0,1,0, 0, 0);
		}
		else
			if(string(argv[i])=="-alength")
		{
			ops.push_back(alength);
		}
		else
		if(string(argv[i])=="-yflip")
		{
			ops.push_back(yflip);
			setmatrix(m,s, 1,0,-1,0, 0, 0);
		}
		else
		if(string(argv[i])=="-xyexchange")
		{
			ops.push_back(xyexchange);
			setmatrix(m,s, 0,1,1,0, 0, 0);
		}
		else
		if(string(argv[i])=="-cw")
		{
			ops.push_back(cw);
			setmatrix(m,s, 0,1,-1,0, 0, 0);
		}
		else
		if(string(argv[i])=="-ccw")
		{
			ops.push_back(ccw);
			setmatrix(m,s, 0,-1,1,0, 0, 0);
		}
		else
		if(string(argv[i])=="-rot")
		{
			ops.push_back(rot);
			if(i++>=argc)
			{
				cerr<<"ERROR: rotation angle required"<<endl;
				exit(1);
			}
			stringstream ss(argv[i]);
			ss>>rotangle;
			rotangle*=pi/180.;
			setmatrix(m,s, cos(rotangle),-sin(rotangle),sin(rotangle),cos(rotangle), 0, 0);
		}
		else
		if(string(argv[i])=="-zxtilt")
		{
			ops.push_back(zxtilt);
			if(i++>=argc)
			{
				cerr<<"ERROR: rotation angle required"<<endl;
				exit(1);
			}
			stringstream ss(argv[i]);
			ss>>rotxangle;
			rotxangle*=pi/180.;
			
		}
		else
		if(string(argv[i])=="-zytilt")
		{
			ops.push_back(zytilt);
			if(i++>=argc)
			{
				cerr<<"ERROR: rotation angle required"<<endl;
				exit(1);
			}
			stringstream ss(argv[i]);
			ss>>rotyangle;
			rotyangle*=pi/180.;
			
		}
		else
		if(string(argv[i])=="-makeabsolut")
		{
			ops.push_back(makeabsolut);
		}
		else
		if(string(argv[i])=="-scale")
		{
			ops.push_back(scale);
			if(i++>=argc)
			{
				cerr<<"ERROR: scale factor required"<<endl;
				exit(1);
			}
			stringstream ss(argv[i]);
			ss>>scalefactor;
			setmatrix(m,s,scalefactor,0,0,scalefactor, 0, 0);
		}
		else
		if(string(argv[i])=="-knive")
		{
			ops.push_back(knive);
			if(i++>=argc)
			{
				cerr<<"ERROR: knive delay required"<<endl;
				exit(1);
			}
			stringstream ss(argv[i]);
			ss>>knivedelay;
			setmatrix(m,s,scalefactor,0,0,scalefactor, 0, 0);
			//cerr<<"knivedelay:"<<knivedelay<<endl;
		}
		else
		if(string(argv[i])=="-killn" ||string(argv[i])=="-killN")
		{
		  ops.push_back(killN);
		}
		else
		if(string(argv[i])=="-parameterize" )
		{
		  ops.push_back(parameterize);
		  if(i++>=argc)
		  {
			 cerr<<"ERROR: min occurence needed for parameterization"<<endl;
			 exit(1);
		  }
		  stringstream ss;
		  ss<<argv[i];
		  if(i++>=argc)
		  {
			 cerr<<"ERROR: start number of variables needed for parameterization"<<endl;
			 exit(1);
		  }
		  ss<<" "<<argv[i];
		  ss>>parm_minoccurence;
		  ss>>parm_startnr;
		  
		  cerr<<"Startnumber:"<<parm_startnr<<" needed occurence "<<parm_minoccurence<<endl;;
		  
		}
		else
		if(string(argv[i])=="-shift")
		{
			ops.push_back(shift);
			double xs,ys;
			if(i++>=argc)
			{
				cerr<<"ERROR: x shift required for shifting operations"<<endl;
				exit(1);
			}
			stringstream ss(argv[i]);
			ss>>xs;
			if(i++>=argc)
			{
				cerr<<"ERROR: y shift required for shifting operations"<<endl;
				exit(1);
			}
			ss.clear();
			ss>>ys;
			setmatrix(m,s, 1,0,0,1, xs, ys);
		}
		else
		if(string(argv[i])=="-comment")
		{
			ops.push_back(comment);
			
			if(i++>=argc)
			{
				cerr<<"ERROR: comment tag required"<<endl;
				exit(1);
			}
			stringstream ss(argv[i]);
			commenttype=ss.get();
			ss>>commenttext;
			
		}
		else
		if(string(argv[i])=="-copies")
		{
			ops.push_back(copies);
			if(string(argv[i+1])!="respect")
			{
				copiesRespect=false;
				stringstream ss;
				if(i++>=argc)
				{
					cerr<<"ERROR: number of x copies required for copies operations"<<endl;
					exit(1);
				}
				ss<<argv[i]<<" ";
				if(i++>=argc)
				{
					cerr<<"ERROR: number of y copies required for copies operations"<<endl;
					exit(1);
				}
				ss<<argv[i]<<" ";
				if(i++>=argc)
				{
					cerr<<"ERROR: x shift required for copies operations"<<endl;
					exit(1);
				}
				ss<<argv[i]<<" ";
				if(i++>=argc)
				{
					cerr<<"ERROR: y shift required for copies operations"<<endl;
					exit(1);
				}
				ss<<argv[i]<<" ";
				
				ss>>nrcopy[0];
				ss>>nrcopy[1];
				ss>>copyshift[0];
				ss>>copyshift[1];
			}
			else
			{
				i++; //first one="respect"
				copiesRespect=true;
				stringstream ss;
				if(i++>=argc)
				{
					cerr<<"ERROR: number of x copies required for copies operations"<<endl;
					exit(1);
				}
				ss<<argv[i]<<" ";
				if(i++>=argc)
				{
					cerr<<"ERROR: number of y copies required for copies operations"<<endl;
					exit(1);
				}
				ss<<argv[i]<<" ";
				if(i++>=argc)
				{
					cerr<<"ERROR: respect distance required for copies operations"<<endl;
					exit(1);
				}
				ss<<argv[i]<<" ";
				ss>>nrcopy[0];
				ss>>nrcopy[1];
				ss>>copiesRespectDistance;
			}
			setmatrix(m,s, 1,0,0,1, 0, 0);
		}
		else
		if(string(argv[i])=="-overlay")
		{
			ops.push_back(overlay);
		  stringstream ss;
		  if(argc<i+8)
		  {
				cerr<<"ERROR: eight coordinates required for overlay!"<<endl;
				exit(1);
		  }
		  ss<<argv[++i]<<" ";ss<<argv[++i]<<" "; ss<<argv[++i]<<" ";ss<<argv[++i]<<" ";
		  ss<<argv[++i]<<" ";ss<<argv[++i]<<" "; ss<<argv[++i]<<" ";ss<<argv[++i]<<" ";
		  ss>>opoints.ax;  ss>>opoints.ay;
		  ss>>opoints.bx;  ss>>opoints.by;
		  ss>>opoints.nax; ss>>opoints.nay;
		  ss>>opoints.nbx; ss>>opoints.nby;
		  //cerr<<opoints.ax<<" "<<opoints.ay <<" "<<opoints.bx<<" "<<opoints.by<<"   "<<opoints.nax<<" "<<opoints.nay <<" "<<opoints.nbx<<" "<<opoints.nby<<endl;
			/*
		  float xref,yref;
		  xref=-(opoints.ax+opoints.bx-opoints.nax-opoints.nbx)/2.; //shift determined by the center points
		  yref=-(opoints.ay+opoints.by-opoints.nay-opoints.nby)/2.; //shift determined by the center points
		  cerr<<"ref: "<<xref<<" "<<yref<<endl;
		  */
		  float cosinus,sinus;
		  float dx,dy,ndx,ndy;
		  dx=opoints.bx-opoints.ax;
		  dy=opoints.by-opoints.ay;
		  ndx=opoints.nbx-opoints.nax;
		  ndy=opoints.nby-opoints.nay;
		  //cerr<<"old "<<dx<<" "<<dy<<"  new"<<ndx<<" "<<ndy<<endl;
		  float factor=1./sqrt(dx*dx+dy*dy)/sqrt(ndx*ndx+ndy*ndy);
		  //cerr<<"Faktor "<<factor<<endl;
		  cosinus=(dx*ndx+dy*ndy)*factor; //dot product
		  sinus=-(dx*ndy-dy*ndx)*factor ;

		  //rotation matrix not any more around reference point, but origin
		  //shift = xref -matrix*xref
		  //s[0]=+xref+(cosinus*xref + sinus*yref);
		  //s[1]=+yref+(-sinus*xref + cosinus*yref);
		  s[0]=(opoints.nbx+opoints.nax)/2-cosinus*((opoints.bx+opoints.ax)/2)-sinus*((opoints.by+opoints.ay)/2);
		  s[1]=(opoints.nby+opoints.nay)/2-cosinus*((opoints.by+opoints.ay)/2)+sinus*((opoints.bx+opoints.ax)/2);
		  m[0]=cosinus;m[1]=sinus;
		  m[2]=-sinus;m[3]=cosinus;
		  
		  cerr<<"rotation: cos "<<cosinus<<" sin "<<sinus<<"   shift "<<s[0]<<" "<<s[1]<<endl;
		  cerr<<"testing: "<<opoints.ax<<" "<<opoints.ay<<" transforms to "<<s[0]+m[0]*opoints.ax+m[1]*opoints.ay<<" "<<s[1]+m[2]*opoints.ax+m[3]*opoints.ay<<endl;
		  cerr<<"testing: "<<opoints.bx<<" "<<opoints.by<<" transforms to "<<s[0]+m[0]*opoints.bx+m[1]*opoints.by<<" "<<s[1]+m[2]*opoints.bx+m[3]*opoints.by<<endl;
		  cerr<<"length discrepancy:"<<sqrt(dx*dx+dy*dy)-sqrt(ndx*ndx+ndy*ndy)<<endl;
		  

		}
		else
		if(string(argv[i])=="-align")
		{
			ops.push_back(align);
			if(i++>=argc)
			{
				cerr<<"align x needed"<<endl;
				exit(1);
			}
			if(string(argv[i])=="min")
			 alignx=align_min;
			else if(string(argv[i])=="max")
			 alignx=align_max;
			else if(string(argv[i])=="middle")
			 alignx=align_middle;
			else if(string(argv[i])=="cmin")
			 alignx=align_cmin;
			else if(string(argv[i])=="cmax")
			 alignx=align_cmax;
			else if(string(argv[i])=="cmiddle")
			 alignx=align_cmiddle;
			else if(string(argv[i])=="keep")
			 alignx=align_keep;
			else
			{
				cerr<<"Error: alignment not supported: \""<<argv[i]<<"\""<<endl;
				exit(1);
			}
			if(i++>=argc)
			{
				cerr<<"align y needed"<<endl;
				exit(1);
			}
			if(string(argv[i])=="min")
			 aligny=align_min;
			else if(string(argv[i])=="max")
			 aligny=align_max;
			else if(string(argv[i])=="center")
			 aligny=align_middle;
			else if(string(argv[i])=="cmin")
			 aligny=align_cmin;
			else if(string(argv[i])=="cmax")
			 aligny=align_cmax;
			else if(string(argv[i])=="cmiddle")
			 aligny=align_cmiddle;
			else if(string(argv[i])=="keep")
			 aligny=align_keep;
			else
			{
				cerr<<"Error: alignment not supported: \""<<argv[i]<<"\""<<endl;
				exit(1);
			}


			setmatrix(m,s,scalefactor,0,0,scalefactor, 0, 0);
			cerr<<"knivedelay:"<<knivedelay<<endl;
		}
		else
		if(string(argv[i])=="-o")
		{
		  if(i++>=argc)
			{
				cerr<<"output file name missing"<<endl;
				exit(1);
			}
			outputfile=argv[i];
		  //i++;
		}
		else
		  if(string(argv[i])=="-g")
		{
		  cerr<<"Outputting gnuplot file"<<endl;
		  if(i++>=argc)
			{
				cerr<<"gnuplot output file name missing"<<endl;
				exit(1);
			}
			outputfilegnuplot=argv[i];
		  //i++;
		}
		else
		{
			if(inputfile.size()==0 && argv[i][0]!='-')
			{
			 inputfile=argv[i];
			 cerr<<"Reading from input file:\""<<inputfile<<"\""<<endl;
			}
			else
			{
			 cerr<<"I do not understand the option \""<<argv[i]<<"\""<<endl;
			 exit(1);
			}
		}
	}
}
int main(int argc, char** argv)
{
	
	if(argc==1)
	{
		cerr<<endl<<"GRECODE can modify GCODE for machining operations."<<endl;
		cerr<<"It is licensed under the GNU Public License."<<endl<<endl;
		cerr<<"Usage:"<<endl;
		cerr<<" grecode <operation [optional value]>  [-o output_gcode.ngc]"<<endl;
		cerr<<"  [ input_gcode.ngc]  [-g output.gnuplot]"<<endl;
		cerr<<endl;
		cerr<<"Operations:"<<endl;
		//none,xflip, yflip, xyexchange, cw,ccw,rot,shift,scale,knive,align,killN,parameterize,overlay,makeabsolut
		cerr<<"-xflip, -yflip,-xyexchange"<<endl;
		cerr<<" just replace the x-y coordinates."<<endl;
		cerr<<"-cw,-ccw"<<endl;
		cerr<<" clockwise or counter-clockwise rotation by 90 degree."<<endl;
		cerr<<"-rot <angle>"<<endl;
		cerr<<" Counter-clockwise Rotation by free angle in degree."<<endl;
		cerr<<" Expressions are not allowed"<<endl;
		cerr<<"-scale <factor>"<<endl;
		cerr<<" Scales the geometry by a factor."<<endl;
		cerr<<"-shift <xshift> <yshift>"<<endl;
		cerr<<" Moves into +x +y by the values in mm."<<endl;
		cerr<<"-align <alignx> <alingy>"<<endl;
		cerr<<" calculates the bounding box by g1 and g0 moves. Arcs are ignored."<<endl;
		cerr<<" alignments are min,middle,max for the G1 and G0total bounding box."<<endl;
		cerr<<" alignments are cmin,cmiddle,cmax for the G1 bounding box."<<endl;
		cerr<<"  Also 'keep' is valid for no shift."<<endl;
		cerr<<"-killn"<<endl;
		cerr<<" removes all N Statements"<<endl;
		cerr<<"-parameterize <minoccurence> <variables Startnumber>"<<endl;
		cerr<<" This will scan for re-occuring values in X, Y and Z words."<<endl;
		cerr<<" If the occure more often than minoccurence,"<<endl;
		cerr<<"  they will be substituted by variables."<<endl;
		cerr<<" Their numbers are starting from the specified number"<<endl;
		cerr<<"-overlay <X PointA> <Y PointA> <X PointB> <Y PointB>"<<endl;
		cerr<<"        <X NewPointA> <Y NewPointA> <X NewPointB> <Y NewPointB>"<<endl;
		cerr<<" This will shift and rotate the the gcode,"<<endl;
		cerr<<" so that PointA and PointB move to the new locations."<<endl;
		cerr<<" Distance mismatches beweeen A-B and newA-newB are compensated."<<endl;
		cerr<<"-knive <delay mm>"<<endl;
		cerr<<" This should compensate partially for foil cutters,"<<endl;
		cerr<<" where the cutting point is lagging."<<endl;
		cerr<<" The lagging distance should be specified."<<endl;
		cerr<<" Arc movements could be problematic currently."<<endl;
		cerr<<"-copies <number x=n> <number y=m> <shiftx> <shifty>"<<endl;
		cerr<<" Creates multiple copies of the original code."<<endl;
		cerr<<" They are aligned in an n times m grid."<<endl;
		cerr<<"-comment <character type><text> "<<endl;
		cerr<<" Comments out words: -comment M03: M03->(M03)"<<endl;
		cerr<<"-zxtilt <angle>  or -zytilt <angle>"<<endl;
		cerr<<" shear-transform z values so that the x-y area is tiltet do the angle"<<endl;
		cerr<<endl;
		cerr<<"Input/Output:"<<endl;
		cerr<<" The program reads input from the console and outputs to the console"<<endl;
		cerr<<" If an input file is specified by -i, it is read instead."<<endl;
		cerr<<" If an output file is specified by -o, the output is written there."<<endl;
		cerr<<" A gnuplot-readable output file can be specified by -g."<<endl;
		cerr<<" It can be viewed by gnuplot: splot \"output.gnuplot\" u 1:2:3:4 w lp palette"<<endl;
		
		return 1;
	}

	processParameters(argc,argv);
	
	
	
	
	GDecoder gd;
	Word w;
	//perform input
	istream *in;
	if(inputfile.length()==0)
	{
	 in=&cin;
	}
	else
	{
	  in=new fstream(inputfile.c_str(),fstream::in);
	  if(!in->good())
	  {
		 cerr<<"Cannot read from input file:\""<<inputfile<<"\""<<endl;
		 return 1;
	  }
	}
	
	//start reading
	gd.read(*in);
	
	//perorm all operations
	for(int i=0;i<ops.size();i++)
	{
	  operations &op=ops[i];
	 if(op==align)  //align is tranfsered into a shift operation
	 {
		op=shift;
		switch(alignx)
		{
		  case align_min: s[0]=-gd.xmin;break;
		  case align_max: s[0]=-gd.xmax;break;
		  case align_middle: s[0]=-(gd.xmax+gd.xmin)/2.;break;
		  case align_cmin: s[0]=-gd.cxmin;break;
		  case align_cmax: s[0]=-gd.cxmax;break;
		  case align_cmiddle: s[0]=-(gd.cxmax+gd.cxmin)/2.;break;
		  case align_keep: s[0]=0;break;
		}
		switch(aligny)
		{
		  case align_min: s[1]=-gd.ymin;break;
		  case align_max: s[1]=-gd.ymax;break;
		  case align_middle: s[1]=-(gd.ymax+gd.ymin)/2.;break;
		  case align_cmin: s[1]=-gd.cymin;break;
		  case align_cmax: s[1]=-gd.cymax;break;
		  case align_cmiddle: s[1]=-(gd.cymax+gd.cymin)/2.;break;
		  case align_keep: s[1]=0; break;
		}

	 }

	 
	 switch(op)
	 {
		  case xflip: gd.scale(-1,1);break;
		  case yflip: gd.scale(1,-1);break;
		  case shift: gd.shift(s[0],s[1]);break;
		  case xyexchange: gd.xyexchange();break;
		  case cw: gd.xyexchange();gd.scale(1,-1);break;
		  case ccw: gd.xyexchange();gd.scale(-1,1);break;
		  case scale: gd.scale(scalefactor,scalefactor);break;
		  case killN: gd.simplify(true,false); break;
		  case makeabsolut: gd.makeabsolute(); break;
		  case comment: gd.wordcomment(commenttype,commenttext); break;
		  case parameterize:
		  {
			 int offset=0;
			 offset=gd.parameterize(parm_startnr,parm_minoccurence,'X');
			 offset+=gd.parameterize(parm_startnr+offset,parm_minoccurence,'Y');
			 gd.parameterize(parm_startnr+offset,parm_minoccurence,'Z');
		  }
			 break;
		  case overlay: gd.fullmatrix(s,m); break;
		  case rot: gd.fullmatrix(s,m); break;
		  case zxtilt: gd.ztilt(rotxangle,'X'); break;
		  case zytilt: gd.ztilt(rotyangle,'Y'); break;
		  case knive: gd.knive(knivedelay);break;
			case alength: gd.alength();break;
		  case copies: 
			{
				if(copiesRespect)
				{
					copyshift[0]=gd.cxmax-gd.cxmin+copiesRespectDistance;
					copyshift[1]=gd.cymax-gd.cymin+copiesRespectDistance;
					cerr<<"for other transformations use:    -copies "<<nrcopy[0]<<" "<<nrcopy[1]<<" "<<copyshift[0]<<" "<<copyshift[1]<<endl;
				}
				gd.copies(nrcopy,copyshift);
				gd.wordcomment('M',"2");
				gd.wordcomment('M',"5");
				gd.wordcomment('M',"30");
				gd.wd.resize(gd.wd.size()+1);
				gd.wd.back().type='M';
				gd.wd.back().text="2";
				gd.wd.back().isLiteral=false;
				gd.wd.back().isVariableDefine=false;
				//gd.wd.push_back(w);
			}
			break;
		  default:
			 cerr<<"operation not implemented yet:"<<op<<endl;
			 return 1;
	 }
	 gd.calcPositions(); //after each operation
	} //operation loop
	gd.findBounds(); //so in the end, the correct information is written into the output
	
	//output
	ostream *out;
	fstream *fout=0;
	if(outputfile.length()>0)
	{
		
	 fout=new fstream(outputfile.c_str(),fstream::out);
	out=fout;
	 if(!out->good())
	 {
		cerr<<"cannot open output file \""<<outputfile<<"\""<<endl;
		return 1;
	 }
	}
	else
	{
		out=&cout;
	}
	
	gd.output(*out);
	if(fout) fout->close();
	

	ostream *outg;
	if(outputfilegnuplot.length()>0)
	{
	 outg=new fstream(outputfilegnuplot.c_str(),fstream::out);
	 if(!outg->good())
	 {
		cerr<<"cannot open gnuplot output file \""<<outputfile<<"\""<<endl;
		return 1;
	 }
	 gd.calcPositions();
	 gd.outputGnuplot(*outg);
	}
	return 0;
	
}





