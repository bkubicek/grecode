/*
Grecode is a tool to modify gcode for CNC machines.
Copyright (C) 2010 Bernhard Kubicek

This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 3 of the License, or any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program; if not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __GDECODERH
#define __GDECODERH

#include <fstream>
#include <string>
#include <vector>
#include <map>
#define SQR(x) (x)*(x)

const double pi=3.14159265;

struct Word  ///this stores the individual junks of information from the gcode, one word could be M3 or G1 or X1 or (comment) or 'newline'
{
	bool isLiteral;
	char type; /// The character of the  GCODE word, e.g. 'G' or 'F'
	std::string text; ///the literal text of the value of the word. This is the thing that will be outputted. value is just for supplementary
	bool isExpression; /// is [] expression 
	bool isVariable; /// specified with '#', <> not supported yet
	bool isVariableDefine; //#1=value or []
	int varNumber;
	float value; /// only set if its a normal number and not an expression. in the future, would be nice if expressions are interpreted
	float curPos[3]; ///position after this words execution
	float lastPos[3]; /// position before this words execution
	float lastDir[3]; /// direction of last move; for knive-cutting delay compensation
	int linenr;
};

struct Variables{
  std::map<int,bool> isValue; ///is the variable a value or an expression
  std::map<int,double> value; /// the value of the variable
  std::map<int,std::string> text; /// the alternative text of the variable

};

class GDecoder
{
public:
	GDecoder();
	~GDecoder();
	int read(std::istream &in);
   void output(std::ostream &out);
	void outputGnuplot(std::ostream &out); ///output to view path with gnuplot. no arcs supported.
	void findBounds(); /// find bounding boxes 
	void calcPositions(); /// calc coordinate-positions of the gcode-words
	void shift(float dx,float dy);
	void scale(float sx,float sy);
	void ztilt(float rot,char direction);
	void knive(float knivedelay); ///knive cutting compensation
	void alength(); ///knive cutting compensation
	void makeabsolute(); ///relative movements->absolute movements
	void fullmatrix(double shift[2],double m[4]); //full matrix operations+shift: x->M.x+s
	void xyexchange();
	void wordcomment(char type, std::string text);
	int parameterize(int startnumber,int minoccurence,char direction);
	void simplify(bool drop_Nwords,bool drop_comments); 
	void copies(int nrcopies[2],double copyshift[2]); ///create mutliple copies next to each other in x and y direction, corresponding to the array index
	
	std::ostream *infostream; ///debugging information is written here, usually cerr so it is not piped
	
public:
	float xmin,xmax,ymin,ymax;//including fast mvoes
	float cxmin,cxmax,cymin,cymax; //just cutted moves
	
	int gnr;
	std::vector<Word>  wd; ///all the words in the loaded gcode
	Variables vs;  /// the variable storage
private:
	void checkVariable(Word &w);  ///parse the Word w, and update/create #variables
	void checkUnits(Word &w,float &units); /// parse the Word w and check for unit changes
	void checkAbsolute(Word &w,bool &moveAbsolute);/// check wether an change from or to an absolute movement occurs
	void checkBreak(Word &w,int lastxmove, int lastymove,int wordnr,bool &breakchain); /// check weather this word stops a combined move. e.g. G0 x10 y10 vs G0 X10 \n Y10, where the \n would break the chain
	bool debug; ///extra output to cerr
	float evaluate(Word &w); //return the value of the word. X1.23->1.23 or X#5->#5value 
	void parseAfterEqual(std::istream &gc,Word &w);  /// read the variable definition after the equal sign: #1=[#5-3] or #1=1.23
public:
	void modify(Word &w, float scale, float shift=0); /// scale this word and shift it by values. 
	
};

inline float sign(float x) {return x>=0?1:-1;} /// guess what...

#endif
