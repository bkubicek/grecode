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
using namespace std;

GDecoder::GDecoder()
{
	debug=true;
	infostream=&cerr;
}

GDecoder::~GDecoder()
{

}
char upper(char c)
{
	if(c>='a' && c<='z')
	{
		return c+'A'-'a';
	}
	else
		return c;
}

void skipwhite (istream &ss)
{
	while(ss.peek()==' ' ||ss.peek()=='\t')
		ss.get();
}

bool isNumberchar(char c)
{
	if(c>='0' &&c<='9')
		return true;
	if(c=='.'||c=='-'||c=='+')
		return true;
	return false;
}

bool isBigLetter(char c)
{
	return c>='A' && c<='Z';
}

void GDecoder::parseAfterEqual(istream &gc,Word &w)
{
	w.text="";
	if(gc.peek()=='[') //we have an expression
			{
				while(gc.good() && gc.peek()!=']')
					w.text+=gc.get();
				if(gc.peek()==']')
					w.text+=gc.get();
				w.isExpression=true;
				//if(debug) cerr<<"expression:"<<w.text<<endl;
			
			}
			else if(gc.peek()=='#') //parameter
			{
				w.text=gc.get();
				while(gc.good() && isNumberchar(gc.peek()))
					w.text+=gc.get();
				w.isVariable=true;
				//if(debug) cerr<<"parameter:"<<w.text<<endl;
			}
			else // we have a number
			{
				while(gc.good() && isNumberchar(gc.peek()))
					w.text+=gc.get();
				stringstream conv;
				conv<<w.text;
				conv>>w.value;
				if (w.type=='X')
					;//moveto(curPos[0],lastPos[0],w.value);
				else
				if (w.type=='Y')
					;//moveto(curPos[1],lastPos[1],w.value);
				else
				if (w.type=='Z')
					;//moveto(curPos[2],lastPos[2],w.value);
				//if(debug) cerr<<"number:"<<w.text<<endl;
			}
}

void GDecoder::ztilt(float rot,char direction)
{
	float units=1;
	bool moveAbsolute=true;
	float lastx=0,lastz=0;
	float sr=tan(rot);
	for(int i=0;i<wd.size();i++)
	{
		struct Word &w=wd[i];
		checkUnits(w,units);
		checkAbsolute(w,moveAbsolute);
		checkVariable(w);
		
		//cerr<<"w.type="<<w.type<<" "<<w.text<<endl;
		if(w.type=='Y' )
		{
			lastx=evaluate(w)*units;
			struct Word nw;
			nw.type='Z';
			stringstream ss;
			ss<<lastx*sr+lastz;
			
			nw.text=ss.str();
			nw.isLiteral=false;
			nw.isVariableDefine=false;
			wd.insert(wd.begin()+i+1,nw);
			i+=1;
			
			cerr<<"lastx="<<lastx<<endl;
			continue;
		}
			
		if(w.type=='Z' )
		{
			lastz=evaluate(w)*units;
			modify(w,1,lastx*sr);
		}

	

	}

}

void GDecoder::alength()
{
	cerr<<"Alength"<<endl;
	float units=1;
	bool moveAbsolute=true;
	float lastx=0,lastz=0;
	
	
	bool withinmove=false;
	float startPos[3];
	
	int startLine=-1;
	bool wasretracted=false;
	float apos=0;
	for(int i=0;i<wd.size();i++)
	{
		struct Word &w=wd[i];
		checkUnits(w,units);
		checkAbsolute(w,moveAbsolute);
		checkVariable(w);
		
		if(w.type=='G')
			if(w.text=="1")
			{
				withinmove=true;
				startPos[0]=w.lastPos[0];
				startPos[1]=w.lastPos[1];
				startPos[2]=w.lastPos[2];
				cerr<<"InMove"<<startPos[0]<<" "<<startPos[1]<<endl;
				startLine=w.linenr;
				
			}
			if(w.lastPos[2]!=startPos[2])
			{
				bool down=(startPos[2]-w.lastPos[2])<0;
				if(wasretracted && !down)
				{
					//unretract
					Word a,g;
					g.type='G';
					g.isLiteral=true;
					g.value=1;
					g.text="G1 ";
					
					a.type='A';
					a.value=123;
					stringstream ll;
					a.isLiteral=true;
					a.linenr=startLine;
					
					ll<<"A"<<apos<<""<<" ; unretract\n";
					a.text=ll.str();
					/*
					for(int j=0;j<10;j++)
					{
						int p=i-j;
						if (p<0) break;
						if(wd[p].type=='G')
						{
							wd.insert(wd.begin()+p,g);i++;
							wd.insert(wd.begin()+p+1,a);i++;
							
							break;
						}
						
						
					}
					*/
					wd.insert(wd.begin()+i+1,g);i++;
					wd.insert(wd.begin()+i+1,a);i++;
					wasretracted=false;
					
					
					
				}
				else
				if(!wasretracted && down)
				{
					//retract
					Word a,g;
					
					g.isLiteral=true;
					g.type='G';
					g.value=1;
					g.text="G1 ";
					
					a.type='A';
					a.value=123;
					stringstream ll;
					a.isLiteral=true;
					a.linenr=startLine;
					ll<<"A["<<apos<<"-#655]"<<" ; retract\n";
					a.text=ll.str();
					for(int j=0;j<10;j++)
					{
						int p=i-j;
						if (p<0) break;
						if(wd[p].type=='G')
						{
							wd.insert(wd.begin()+p,g);i++;
							wd.insert(wd.begin()+p+1,a);i++;
							break;
							
							
						}
					}
					wasretracted=true;
				}
					
			}
		//if(w.text.find("\n")!=std::string::npos)
		if(withinmove && ((w.linenr!=startLine) || w.text=="\n"))
		{
				withinmove=false;
				float l=sqrt( SQR(startPos[0]-w.lastPos[0])+SQR(startPos[1]-w.lastPos[1]) );
				if(l!=0)
				{
				Word a;
				
				a.type='A';
				
				a.value=l;
				stringstream ll;
				a.isLiteral=true;
				a.linenr=startLine;
				apos+=l;
				ll<<"A["<<apos<<"*#654]"<<" ; "<<startPos[0]<<" "<<startPos[1]<<" ->"<<w.lastPos[0]<<" "<<w.lastPos[1];
				a.text=ll.str();
				
				wd.insert(wd.begin()+i,a);
				i++;
				//cerr<<"EndMove"<<l<<endl;
				}
			
		}
	

	}

}

int GDecoder::read(std::istream &gc)
{
	std::string wholeline; // the text of the complete line
	bool lhasx,lhasy,lhasboth; // line has '' word
  *infostream<<"READING"<<endl;
  int linenr=1;
	while(gc.good())
	{
		//*infostream<<linenr<<endl;
		char c;
		skipwhite(gc);
		c=upper(gc.get());
		
		if(!gc.good())
			break;
		wd.resize(wd.size()+1);
		struct Word &w=wd.back();
		w.isExpression=false;
		w.isVariable=false;
		w.isLiteral=false;
		w.isVariableDefine=false;
		w.linenr=linenr;
		if(isBigLetter(c)) //probably real code
		{
			w.type=c;
			//if(debug) cerr<<"type:\""<<c<<"\""<<endl;
			w.text="";
			
			skipwhite(gc);
			parseAfterEqual(gc,w);
			
			
			//if(debug) cerr<<" word "<<w.type<<": \""<<w.text<<"\""<<endl;
		}  
		else  //not big letter
		{ //store the uninteresting code also as ' ' words, so they can be outputted
			w.isLiteral=true;
			w.type=' ';
			w.text=c;
			string tmp;
			switch(c)
			{
				case ';':
					getline(gc,tmp,'\n');
					w.text+=tmp+"\n";
					linenr++;
					//if(debug) cerr<<" comment text: \""<<w.text<<"\""<<endl;
				break;
				case '(':
					getline(gc,tmp,')');
					w.text+=tmp+')';
					linenr++;
					//if(debug) cerr<<" comment text: \""<<w.text<<"\""<<endl;
				break;
				case 13:  //windows line feed
				case '\n':
					w.text='\n';
					linenr++;
					//if(debug) cerr<<"Newline"<<endl;
				break;
				case '#': //variable declaration
				{
					
					w.isVariableDefine=true;
					w.isLiteral=false;
					w.type=c;
					stringstream ss("");
					while(gc.good() && isNumberchar(gc.peek()))
					{
						ss<<(char)gc.get();
					}
					//*infostream<<"variable number text: \""<<ss.str()<<"\""<<endl;
					ss>>w.varNumber;
					skipwhite(gc);
					char equal=gc.get();
					if(equal!='=')
					{
						*infostream<<"ERROR: expected '=' in variable definition, but got \""<<c<<"\" instead."<<endl;
					}
					//*infostream<<"peek:"<<(char)(gc.peek())<<endl;
					parseAfterEqual(gc,w);
					//*infostream<<"Variable definition: #"<<w.varNumber<<"=\""<<w.text<<"\""<<endl;
				}
				break;
				default:
					getline(gc,w.text);
					w.text+='\n';
					linenr++;
					if(w.text.size()>0)
					{
					  
						*infostream<<" problem with text: tag \""<<c<<"\"   \""<<w.text<<"\""<<endl;
					}
			}
		}
	}
	calcPositions();
	findBounds();
	*infostream<<"input file has "<<linenr<<" lines "<<endl;
}

void GDecoder::calcPositions()
{
  *infostream<<"CALCPOSITIONS"<<endl;
	bool moveAbsolute=true;
	float units=1;
	float curx=0,cury=0,curz=0;
	float newx=0,newy=0,newz=0;
	
	
	for(int i=0;i<wd.size();i++)
	{
		struct Word &w=wd[i];
		checkVariable(w);
		checkUnits(w,units);
		checkAbsolute(w,moveAbsolute);
	 
		w.lastPos[0]=curx;
		w.lastPos[1]=cury;
		w.lastPos[2]=curz;
		newx=curx;newy=cury;newz=curz;
		switch(w.type)
		{
		case 'X':
			if(moveAbsolute)
				newx=evaluate(w)*units;
			else
				newx=curx+evaluate(w)*units;
		
		break;
		case 'Y':
			if(moveAbsolute)
				newy=evaluate(w)*units;
			else
				newy=cury+evaluate(w)*units;
		
		break;
		case 'Z':
			if(moveAbsolute)
				newz=evaluate(w)*units;
			else
				newz=curz+evaluate(w)*units;
		
		break;
		
		default:
			;
		}
		w.curPos[0]=newx;
		w.curPos[1]=newy;
		w.curPos[2]=newz;
		//if(w.type=='X' || w.type=='Y' ||w.type=='Z' )
		;//cout<<curx<<" "<<cury<<" "<<curz<<" "<<units<<endl;
		//*infostream<<"word "<<w.type<<w.text<<" curpos"<<w.curPos[0]<<" "<<w.curPos[1]<<"   lastpos"<<w.lastPos[0]<<" "<<w.lastPos[1]<<endl;
		
		curx=newx;
		cury=newy;
		curz=newz;
		
		
	}
}

void GDecoder::findBounds()
{
	 *infostream<<"FINDBOUNDS"<<endl;
	xmin=ymin=100000;
	xmax=ymax=-100000;
	
	for(int i=0;i<wd.size();i++)
	{
		struct Word &w=wd[i];
		checkVariable(w);
		if(w.type=='X')
		{
			if(w.curPos[0]>xmax)
				xmax=w.curPos[0];
			else
			if(w.curPos[0]<xmin)
				xmin=w.curPos[0];
		}
		
		if(w.type=='Y')
		{
			if(w.curPos[1]>ymax)
				ymax=w.curPos[1];
			else
			if(w.curPos[1]<ymin)
				ymin=w.curPos[1];
		}
	}
	
	cxmin=cymin=100000;
	cxmax=cymax=-100000;
	
	bool isCutting;
	for(int i=0;i<wd.size();i++)
	{
		struct Word &w=wd[i];
		checkVariable(w);
		if(w.type=='G' && (w.text=="01"||w.text=="1"))
			isCutting=true;
		else
		if(w.type=='G' && (w.text=="00"||w.text=="0"))
			isCutting=false;
		if(!isCutting)
			continue;
		
		if(w.type=='X')
		{
			if(w.curPos[0]>cxmax)
				cxmax=w.curPos[0];
			else
			if(w.curPos[0]<cxmin)
				cxmin=w.curPos[0];
		}
		
		if(w.type=='Y')
		{
			if(w.curPos[1]>cymax)
				cymax=w.curPos[1];
			else
			if(w.curPos[1]<cymin)
				cymin=w.curPos[1];
		}
	}
	
	infostream->precision(3);
	*infostream<<"Potential Total Bounds:\n x in \t"<<xmin<<"\t"<<xmax<<"  extension "<<xmax-xmin<<"\n y in \t"<<ymin<<"\t"<<ymax<<"  extension "<<ymax-ymin<<endl;;
	*infostream<<"Potential Cut   Bounds:\n x in \t"<<cxmin<<"\t"<<cxmax<<"  extension "<<cxmax-cxmin<<"\n y in \t"<<cymin<<"\t"<<cymax<<"  extension "<<cymax-cymin<<endl;;
}



void  GDecoder::modify(Word &w, float scale, float shift)
{
	stringstream  ss;
	ss.precision(5);
	ss<<fixed;
	//*infostream<<"Modify:"<<w.type<<w.text<<" "<<scale<<" "<<shift<<endl;
	if(w.isExpression ||w.isVariable)
	{
		ss<<"[";
		if(shift!=0)
			ss<<shift<<"+";
		string add=w.text;
		if(add[0]=='[' && scale==1 )
		{
			//cout<<"ADD:"<<add;
			add.erase(w.text.size()-1);
			add.erase(0,1);
			//cout<<"  ->"<<add;
		}
		if(scale==-1)
			ss<<"-"<<add;
		else
		if(scale==1)
			ss<<add;
		else
		ss<<scale<<"*"<<w.text;
		ss<<"] ";
	}
	else
		ss<<shift+scale*w.value<<" ";
	w.text=ss.str();
	ss>>w.value;
}


void GDecoder::output(ostream &out)
{
	out.setf(ios_base::fixed);
	out.precision(5);
	out<<fixed;
	//out<<";###############"<<endl;
	//out<<"; possible total bounding box: x in "<<xmin<<" "<<xmax<<"  y in "<<ymin<<" "<<ymax<<endl;
	//out<<"; possible cutting bounding box: x in "<<cxmin<<" "<<cxmax<<"  y in "<<cymin<<" "<<cymax<<endl;
	
	for(int i=0;i<wd.size();i++)
	{
		struct Word &w=wd[i];
		if(w.isLiteral)
			out<<w.text;
		else
		if(w.isVariableDefine)
			out<<w.type<<w.varNumber<<"="<<w.text<<" ";
		else
			out<<w.type<<w.text<<" ";
	}
	//out<<";###############"<<endl;
	//out<<endl;
	
}

void GDecoder::outputGnuplot(ostream &out)
{
	out.setf(ios_base::fixed);
	out.precision(5);
	out<<fixed;
	out<<"# plot using gnuplot:\n#splot \"filename.xy\" u 1:2:3:4 w lp palette"<<endl;
	 int lastxmove=0,lastymove=0,lastzmove=0;
	 bool rapid=false;
	for(int i=0;i<wd.size();i++)
	{
		struct Word &w=wd[i];
		checkVariable(w);
		bool breakchain=false;
		checkBreak(w,lastxmove,lastymove,i, breakchain);
		if(w.type=='Z' && lastzmove!=0)
		  breakchain=true;


		/*
		*infostream<<"Gnuplot: "<<" Word Type '"<<w.type<<"' \""<<w.text<<"\" ";
		if (breakchain)
		  *infostream<<"breaks"<<endl;
		 else
			 *infostream<<"continues"<<endl;
		*infostream<<"lastpos "<<lastxmove<< " "<<lastymove<<endl;
		*/
		

		if(breakchain)
		{
		  out<<w.curPos[0]<<" "<<w.curPos[1]<<" "<<w.curPos[2]<<" ";
		  if(rapid)
			out<<"1";
		  else
			out<<"0";
		  out<<endl;
		  //*infostream<<" curpos: "<<w.curPos[0]<<" "<<w.curPos[1]<<" "<<w.curPos[2]<<endl;;
		  lastxmove=0;
		  lastymove=0;
		}

		if(w.type=='X') lastxmove=i;
		if(w.type=='Y') lastymove=i;
		//if(w.type=='Z') lastymove=i;
		if(w.type=='G' && (w.text=="00"||w.text=="0"))
		  rapid=true;
		if(w.type=='G' && (w.text=="01"||w.text=="1"))
		  rapid=false;


	}
}

void  GDecoder::shift(float dx,float dy)
{
	//cerr<<"Shifting:"<<dx<<" "<<dy<<endl;
	float units=1;
	bool moveAbsolute=true;
	for(int i=0;i<wd.size();i++)
	{
		struct Word &w=wd[i];
		checkUnits(w,units);
		checkAbsolute(w,moveAbsolute);
		
		if(w.type=='X' && dx!=0)
			modify(w,1,dx/units);
		if(w.type=='Y' && dy!=0)
			modify(w,1,dy/units);

	}
}

void  GDecoder::scale(float sx,float sy)
{
	static bool arcMessageDisplayed=false;
	for(int i=0;i<wd.size();i++)
	{
		struct Word &w=wd[i];
		checkVariable(w);
		if(w.type=='X' && sx!=1)
			modify(w,sx);
		if(w.type=='Y' && sy!=1)
			modify(w,sy);
		if(w.type=='G' && (w.text=="2"||w.text=="02") && sx*sy==-1)
		{
			w.text="03";
		}
		else
		if(w.type=='G' && (w.text=="3"||w.text=="03") && sx*sy==-1)
		{
			w.text="02";
		}
		if(w.type=='I' && sx!=1)
		{
			if(sx==sy||sx==1||sx==-1)
				modify(w,sx);
			else
			{
				if(!arcMessageDisplayed)
				*infostream<<"PROBLEM: arcs cannot not be scaled arbitrarily"<<endl;
				arcMessageDisplayed=true;
			}
			
		}
		if(w.type=='J' && sy!=1)
		{
			if(sx==sy||sy==1||sy==-1)
				modify(w,sy);
			else
			{
				if(!arcMessageDisplayed)
				*infostream<<"PROBLEM: arcs cannot not be scaled arbitrarily"<<endl;
				arcMessageDisplayed=true;
			}
			
		}

	}
}

void  GDecoder::xyexchange()
{
	for(int i=0;i<wd.size();i++)
	{
		struct Word &w=wd[i];
		switch(w.type)
		{
		case 'X': w.type='Y';break;
		case 'Y': w.type='X';break;
		case 'I': w.type='J';break;
		case 'J': w.type='I';break;
		case 'G':
			if(w.text=="02"||w.text=="2")
			{
				w.text="03";
			}
			else
			if(w.text=="03"||w.text=="3")
			{
				w.text="02";
			}
		break;
		default:
			;
		}
		
	}
}


float GDecoder::evaluate(Word &w)
{
	static bool expErrorShowed=false;
	static bool varErrorShowed=false;
	if(w.isExpression)
	{
		if(!expErrorShowed)
		*infostream<<"Bounding box ERROR: Expression substitution not implemented yet"<<endl;
		expErrorShowed=true;
		return 0;
	}	
	
	
	if(w.isVariable)
	{
	  stringstream ss(w.text);
	  ss.get();
	  int varnumber;
	  ss>>varnumber;
	  if(vs.isValue[varnumber])
	  {
		  //*infostream<<"replacing variable "<<varnumber<<" by "<<vs.value[varnumber]<<endl;
		  return vs.value[varnumber];
	  }
		if(!varErrorShowed)
		{
		  *infostream<<"Bounding box ERROR: Variable substitution not implemented yet"<<endl;
		  *infostream<<"Word: "<<w.type<<" "<<w.text<<endl;
		}
		varErrorShowed=true;
		return 0;
	}	
	if(w.isLiteral)
	{
		return 0;
	}
	if(fabs(w.value)>1000)
	{
			*infostream<<"Warning very large value in"<<endl;
		  *infostream<<"Word: "<<w.type<<" "<<w.text<<endl;
	}
	return w.value;
}

int GDecoder::parameterize(int startnumber,int minoccurence,char direction)
{
  vector<float> vals;
  vector<float> valcount;
  vals.reserve(1000);
  valcount.reserve(1000);
  //count occurences of value fields
  for(int i=0;i<wd.size();i++)
  {
		struct Word &w=wd[i];
		if(w.type==direction && !w.isExpression && !w.isVariable)
		{
			
		  bool found=false;
		  for(int j=0;j<vals.size();j++)
		  {
			 if(vals[j]==w.value)
			 {
				  valcount[j]++;
				  found=true;
				 // *infostream<<"tag again:"<<w.value<<" "<<valcount[j]<<endl;
				  break;
			 }
		  }
		  if(!found)
		  {
			 //*infostream<<"new tag:"<<w.value<<endl;
			 vals.push_back(w.value);
			 valcount.push_back(0);
		  }
		}
  }
  vector<float> parameters;
 
  parameters.reserve(100);
  int insertcount=0;
  //find appropriate parameters
  for(int j=0;j<vals.size();j++)
  {
	 
	 if(valcount[j]>minoccurence)
	 {
		  parameters.push_back(vals[j]);
		  Word nw;
		  nw.type=' ';
		  nw.isVariable=false; //not a single variable in a word, but a defininition
		  nw.isExpression=false;
		  nw.isLiteral=true;
		  stringstream ss;
		  ss<<"#"<<startnumber+parameters.size()-1<<"="<<vals[j]<<"   ; auto direction "<<direction<<" occurences:"<< valcount[j]<<" value="<<vals[j]<<endl;
		  nw.text=ss.str();
		  wd.insert(wd.begin()+insertcount++,nw);
		  *infostream<<"found parameter:"<<vals[j]<<endl;
		  
	 }
	}

	//insert parameters
	for(int i=0;i<wd.size();i++)
	{
		struct Word &w=wd[i];
		if(w.type==direction && !w.isExpression && !w.isVariable)
		{
		  for(int j=0;j<parameters.size();j++)
		  if(w.value==parameters[j])
		  {
			 w.isVariable=true;
			 stringstream ss;
			 ss<<"#"<<startnumber+j;
			 w.text=ss.str();
			 break;
		  }

		}
	}
  return parameters.size();
}

void GDecoder::simplify(bool drop_Nwords,bool drop_comments)
{
  
  int i=0;
  do
  {
		 // *infostream<<" now at "<<i<<endl;
		if(drop_Nwords && wd[i].type=='N')
		{
		  wd.erase(wd.begin()+i);
		}
		else if(drop_comments && wd[i].type==';')
		{
		  wd[i].type=' ';
		  wd[i].text='\n';
		  i++;

		}
		else if(drop_comments && wd[i].type=='(')
		{
		  wd.erase(wd.begin()+i);
		}
		else
		  i++;
		 
		  
  }while(i<wd.size());
}

void GDecoder::fullmatrix(double shift[2],double m[4])
{
  int lastxmove=0,lastymove=0;
  int lastimove=0,lastjmove=0;
  
  bool moveAbsolute=true;
  *infostream<<"FULL MATRIX OPERATION:  Shift: "<<shift[0]<<" "<<shift[1]<<"  Matrix "<<m[0]<<" "<<m[1]<<" , "<<m[2]<<" "<<m[3]<<endl;
  float units=1;
  float newunits=1;
  
  bool megadebug=false;
  
  for(int i=0;i<wd.size();i++)
  {
		struct Word &w=wd[i];
		checkUnits(w,newunits);
		checkAbsolute(w,moveAbsolute);
		bool breakchain=false;
		//*infostream<<"Prebreak lastmoves:"<<lastxmove<<" "<<lastymove<<endl;
		checkBreak(w,lastxmove,lastymove,i,breakchain);
		if(megadebug)
		{
			if(w.type==' ' && w.text=="\n")
			{
				if(breakchain)
					*infostream<<"#"<<w.linenr+1<<" endl breaks before"<<endl;
				else
				*infostream<<"#"<<w.linenr+1<<" continues"<<endl;
			}
			else
			{
				*infostream<<"#"<<w.linenr<<" "<<w.type<<w.text;
				if(breakchain)
					*infostream<<" breaks ";
				else
					*infostream<<" continues ";
				
				*infostream<<" curpos "<<w.curPos[0]<<" "<<w.curPos[1];
				*infostream<<" lastpos "<<w.lastPos[0]<<" "<<w.lastPos[1]<<endl;
			}
		}


		//if(w.linenr==7){output(*infostream);exit(1);}
		if(lastxmove==0 &&lastymove==0)
			breakchain=false;
		if(breakchain)  //end of a unified move found
		{
			
			if(w.isExpression && (w.type=='X'||w.type=='Y'||w.type=='Z'))
			{
				cerr<<"Cannot only rotate free with no gcode expressions [] !"<<endl;
				return ;
			}
			if(lastxmove==0||lastymove==0) //we have to insert one more word as rotated needs more than the specified words
			{
			  
				Word iw;//instertword

				//*infostream<<"lasts:"<<lastxmove<<" "<<lastymove<<" inserting between:"<<wd[i-1].type<<wd[i-1].text<<" and "<<wd[i].type<<wd[i].text<<endl;
				std::vector<Word>::iterator it=wd.begin();
				//*infostream<<"wd.size before "<<wd.size()<<endl;
				wd.insert(it+i,iw);
				//*infostream<<"wd.size after"<<wd.size()<<endl;
				int neww=i;
				//*infostream<<"inserting at"<<i<<endl;
				i++;
				
				
				wd[neww].isExpression=false;
				wd[neww].isVariable=false;
				wd[neww].isLiteral=false;
				wd[neww].isVariableDefine=false;
				wd[neww].linenr=wd[i].linenr;
				wd[neww].type='(';
				wd[neww].text="()";
				wd[neww].value=0;
				
				if(lastxmove==0)
				{
					wd[neww].type='X';
					
					lastxmove=neww;
					wd[neww].value=wd[i].lastPos[0];
					stringstream ss;
					ss<<wd[neww].value;
					
					wd[neww].text=ss.str();
					//modify(wd[lastxmove],0,wd[i].lastPos[0]);
					
				}
				else
				{
				
				  wd[neww].type='Y';
				  
				  lastymove=neww;
				  stringstream ss;
				  wd[neww].value=wd[i].lastPos[1];
					ss<<wd[neww].value;
					wd[neww].text=ss.str();
				  //modify(wd[lastymove],0,wd[i].lastPos[1]);
				  
				}
				wd[neww].lastPos[0]=wd[i].lastPos[0];
				wd[neww].lastPos[1]=wd[i].lastPos[1];
				wd[neww].lastPos[2]=wd[i].lastPos[2];
				//*infostream<<"newword:"<<wd[neww].text<<endl;
				/*
				for(int g=5;g>=-1;g--)
				{
				  *infostream<<"->";
				  if(i-g==neww) *infostream<<" * ";
				 *infostream<<wd[i-g].type<<" "<<wd[i-g].text<<"  ";
				}
				*infostream<<endl;
				*/
				

			}
			//*infostream<<"lastx:"<<wd[lastxmove].type<<wd[lastxmove].text<<" lasty:"<<wd[lastymove].type<<wd[lastymove].text<<endl;
			if(wd[lastxmove].type!='X')
			 exit(1);
			if(wd[lastymove].type!='Y')
			 exit(1);
			
			if(moveAbsolute)
			{
				if(lastxmove>0)
				modify(wd[lastxmove],m[0],(shift[0]+m[1]*w.lastPos[1])/units);
				if(lastymove>0)
				modify(wd[lastymove],m[3],(shift[1]+m[2]*w.lastPos[0])/units);
			}
				else
			{
				double wdlx=0,wdly=0;
				if(lastxmove>0)
					wdlx=evaluate(wd[lastxmove]);
				if(lastymove>0)
					wdly=evaluate(wd[lastymove]);
				
				if(lastxmove>0)
				modify(wd[lastxmove],m[0],m[1]*wdly);
				if(lastymove>0)
				modify(wd[lastymove],m[3],m[2]*wdlx);
			}
		  
		  //arcs
				if(lastimove!=0 || lastjmove!=0)
			{
		  		double wdlx=0,wdly=0;
		  		if(lastimove)
					wdlx=evaluate(wd[lastimove]);
				if(lastjmove)
					wdly=evaluate(wd[lastjmove]);
					
				modify(wd[lastimove],m[0],m[1]*wdly/units);
				modify(wd[lastjmove],m[3],m[2]*wdlx/units);
		  }

		  
		  
		  lastxmove=0;lastymove=0;
		  lastimove=0;lastjmove=0;
		  //*infostream<<"Emptied"<<endl;
		}
		units=newunits;
		if(wd[i].type=='X') lastxmove=i;
		if(wd[i].type=='Y') lastymove=i;
		if(wd[i].type=='I') lastimove=i;
		if(wd[i].type=='J') lastjmove=i;
	}
}
void  GDecoder::checkAbsolute(Word &w,bool &moveAbsolute)
{
	if(w.type=='G')
	{
		if(w.text=="90")
		{
			moveAbsolute=true;
		}
		if(w.text=="91")
		{
			moveAbsolute=false;
		}
	}
}

void GDecoder::checkUnits(Word &w,float &units)
{
	bool showdebug=false;
	if(w.type=='G')
	{
		if(w.text=="20")
		{
			if(showdebug) *infostream<<"Switching to inch"<<endl;
			units=25.4;
			
		}
		else
		if(w.text=="21")
		{
			units=1;
			if(showdebug) *infostream<<"Switching to mm"<<endl;
		}
	}
}

void GDecoder::makeabsolute()
{
	*infostream<<"MAKEABSOLUT"<<endl;
	//bool debug=true;
  bool moveAbsolute=true;
  	float units=1;
	float curx=0,cury=0,curz=0;
	float newx=0,newy=0,newz=0;
	//theses are always in mm;
  for(int i=0;i<wd.size();i++)
  {
		struct Word &w=wd[i];
		checkVariable(w);
		
		if(w.type=='G' && w.text=="20" && fabs(units-25.4)>0.1)
		{
			checkUnits(w,units);
			if(debug) *infostream<<"Switching to inch in line"<<w.linenr<<" "<<units<<" :"<<curx<<" "<<cury<<" "<<curz<<endl;
			//curx*=1/units;
			//cury*=1/units;
			//curz*=1/units;
			
			
			//if(debug) *infostream<<curx<<" "<<cury<<" "<<curz<<endl;
		}
		if(w.type=='G' && w.text=="21" && fabs(units-25.4)<0.1)
		{
			checkUnits(w,units);
			if(debug) *infostream<<"Switching to mm "<<curx<<" "<<cury<<" "<<curz<<endl;
			//curx*=units;
			//cury*=units;
			//curz*=units;
			///if(debug) *infostream<<curx<<" "<<cury<<" "<<curz<<endl;
		}
		
		checkAbsolute(w,moveAbsolute);
		
		if(w.type=='G' && w.text=="91")
			w.text="90";


		newx=curx;newy=cury;newz=curz;
		switch(w.type)
		{
		case 'X':
			if(moveAbsolute)
				newx=evaluate(w)*units;
			else
				newx=curx+evaluate(w)*units;

		break;
		case 'Y':
			if(moveAbsolute)
				newy=evaluate(w)*units;
			else
				newy=cury+evaluate(w)*units;

		break;
		case 'Z':
			if(moveAbsolute)
				newz=evaluate(w)*units;
			else
				newz=curz+evaluate(w)*units;

		break;

		default:
			;
		}
		if(!moveAbsolute)
		{
		  switch(w.type)
		  {
			 case 'X': modify(w,0,newx/units);break;
			 case 'Y': modify(w,0,newy/units);break;
			 case 'Z': modify(w,0,newz/units);break;
			 default:
				;
		  }
			
		}
		if(0&&(w.type=='Z'))
			{
				stringstream ss;
				
				ss<<"("<<curz<<"->"<<newz<<")";
				w.text+=ss.str();
			}
		curx=newx;
		cury=newy;
		curz=newz;

  }
  calcPositions();
}

void  GDecoder::checkBreak(Word &w,int lastxmove, int lastymove,int wordnr,bool &breakchain)
{
	bool thisdebug=false;
	if(lastxmove==0 && lastymove ==0)
		breakchain=false;
	else
	{
		if(w.type=='G')
		{
			breakchain=true;
			if(thisdebug) *infostream<<"Breaks due to G"<<endl;
		}
		if(w.type==' ' && w.text=="\n")
		{
			breakchain=true;
			if(thisdebug) *infostream<<"Breaks due to newl"<<endl;
		}
		if(w.type=='X' && lastxmove>0)
		{
			breakchain=true;
			if(thisdebug) *infostream<<"Breaks due to next X"<<endl;
		}
		if(w.type=='Y' && lastymove>0)
		{
			breakchain=true;
			if(thisdebug) *infostream<<"Breaks due to next Y"<<endl;
		}
		if(wordnr==wd.size()-1)
		{
			breakchain=true;
			if(thisdebug) *infostream<<"Breaks due to final statement"<<endl;
		}
	}

}

void GDecoder::checkVariable(Word& w)
{
  if(w.isVariableDefine)
  {
	 vs.text[w.varNumber]=w.text;
	 stringstream ss;
	 double val=123456789;
	 ss<<w.text;
	 ss>>val;
	 vs.isValue[w.varNumber]=false;;
	 if(val!=123456789)
	 {
		vs.value[w.varNumber]=val;
		vs.isValue[w.varNumber]=true;
	 }
	// *infostream<<"Variable change: #"<<w.varNumber<<"=\""<<vs.text[w.varNumber]<<"\" = "<<vs.value[w.varNumber]<<endl;
  }
}

void GDecoder::knive(float knivedelay)
{
  *infostream<<"KNIVE COMPENSATION"<<endl;
	bool moveAbsolute=true;
  float units=1;
  float dir[2]={0,0};
  int lastbreak=0,lastxmove=0,lastymove=0;
  for(int i=0;i<wd.size();i++)
  {
		struct Word &w=wd[i];
		checkVariable(w);
		checkUnits(w,units);
		bool breakchain=false;
		//*infostream<<"Prebreak lastmoves:"<<lastxmove<<" "<<lastymove<<endl;
		checkBreak(w,lastxmove,lastymove,i,breakchain);
		if(breakchain)
		{
		  dir[0]=wd[i].curPos[0]-wd[lastbreak].curPos[0];
		  dir[1]=wd[i].curPos[1]-wd[lastbreak].curPos[1];
		  float length=sqrt(SQR(dir[0])+SQR(dir[1]));
		  if(length>0)
		  {
			 dir[0]*=1/length;
			 dir[1]*=1/length;
		  }
		  if(lastxmove>0)
			 modify(wd[lastxmove],1,dir[0]*knivedelay);
		  if(lastymove>0)
			 modify(wd[lastymove],1,dir[1]*knivedelay);
		  lastbreak=i;
		  lastxmove=0;
		  lastymove=0;
		}
		if(w.type=='X') lastxmove=i;
		if(w.type=='Y') lastymove=i;

  }
}

void copyWord(const Word &from, Word &to)
{
	to.isLiteral=from.isLiteral;
	to.type=from.type;
	to.text=from.text;
	to.isExpression=from.isExpression;
	to.isVariable=from.isVariable;
	to.isVariableDefine=from.isVariableDefine;
	to.varNumber=from.varNumber;
	to.value=from.value;
	to.curPos[0]=from.curPos[0];
	to.curPos[1]=from.curPos[1];
	to.curPos[2]=from.curPos[2];
	to.lastPos[0]=from.lastPos[0];
	to.lastPos[1]=from.lastPos[1];
	to.lastPos[2]=from.lastPos[2];
	to.lastDir[0]=from.lastDir[0];
	to.lastDir[1]=from.lastDir[1];
	to.lastDir[2]=from.lastDir[2];
	to.linenr=from.linenr;

}

void GDecoder::copies(int nrcopies[2],double copyshift[2])
{
  int oldlength=wd.size();
  if( (nrcopies[0]<=0) ||(nrcopies[1]<=0))
  {
	 *infostream<<"Error: copies number not positive"<<endl;
	 *infostream<<"Nothing done"<<endl;
	 return;
  }
  wd.resize(oldlength*nrcopies[0]*nrcopies[1]);
  for(int nx=0;nx<nrcopies[0];nx++)
  for(int ny=0;ny<nrcopies[1];ny++)
  {
		if(nx+ny==0)
		  continue;  //the first copy is the original

		float units=1;
		bool moveAbsolute=true;
		for(int i=0;i<oldlength;i++)
		{
		  struct Word &oldw=wd[i];
		  struct Word &neww=wd[i+(ny*nrcopies[0]+nx)*oldlength];
		  copyWord(oldw,neww);
		  //*infostream<<"Oldi "<<i<<" New i "<<i+(ny*nrcopies[0]+nx)*oldlength<<endl;
		  
			checkUnits(oldw,units);
			checkAbsolute(oldw,moveAbsolute);
			double dx,dy;
			dx=copyshift[0]*nx;
			dy=copyshift[1]*ny;
			if(neww.type=='X' && dx!=0)
			  modify(neww,1,dx/units);
			if(neww.type=='Y' && dy!=0)
			  modify(neww,1,dy/units);
		}
  }

}

void GDecoder::wordcomment(char type, std::string text)
{
  *infostream<<"Disabling "<<type<<text<<endl;
  for(int i=0;i<wd.size();i++)
  {
	 struct Word &w=wd[i];
	 if(w.type==type)
	 if(w.text==text)
	 {
		string stype=" ";
		stype[0]=w.type;
		w.text=stype+w.text+string(")");
		w.type='(';
	 }
  }

}
