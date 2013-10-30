grecode
=======

A program to manipuate gcode

For many CNC machining applications it might be required to modify the gcode.

grecode can shift, rotate, mirror, align gcode.

It is called from the command line. By executing it without parameters, a hopefully usefull help message will be displayed:

Usage: grecode <operation [optional value]> [-o output_gcode.ngc] [ input_gcode.ngc] [-g output.gnuplot]
Operations:

    -xflip
        inverts all X coordinates and expressions 
    -yflip
        inverts all Y coordinates and expressions 
    -xyexchange
        just replace the X and Y coordinates and expressions. Also I and J of arcs 
    -cw
    -ccw
        clockwise or counter-clockwise rotation by 90 degree. 
    -rot angle
        Counter-clockwise Rotation by free angle in degree. Expressions are not allowed 
    -scale factor
        Scales the geometry by a factor. 
    -shift xshift yshift
        Moves into +x +y by the values in mm. 
    -align alignx alingy
        calculates the bounding box by g1 and g0 moves. Arcs are ignored. Alignments are min,middle,max for the G1 and G0 total bounding box; cmin,cmiddle,cmax for the G1 bounding box. Also 'keep' is valid for no shift. 
    -killn
        removes all N Statements 
    -parameterize minoccurence variablesStartnumber
        This will scan for re-occuring values in X, Y and Z words. If the occure more often than minoccurence, they will be substituted by variables. Their numbers are starting from the specified number 
    -overlay XPointA YPointA XPointB YPointB XNewPointA YNewPointA XNewPointB YNewPointB
        This will shift and rotate the the gcode so that PointA and PointB move to the new locations. Distance mismatches beweeen A-B and newA-newB are compensated. 
    -knive <delay mm>
        This should compensate partially for foil cutters, where the cutting point is lagging. The lagging distance should be specified in mm. Arc movements could be problematic currently. The implementation is not very good. 
    -copies amountOfCopiesX amountOfCopiesY shiftx shifty
        Creates multiple copies of the original code. They are aligned in an n times m grid. Optimal for creating batches of parts. However, End program statements and such should be removed by the -comment option 
    -makeabsolut Recalculate paths from relative moves to absolute moves.
    -comment Word
        Comments out words: Example -comment M03 will replace all M03 by (M03) 
    -zxtilt angle or -zytilt angle
        shear-transform z values so that the x-y area is tilted do the angle 

Input/Output:

    The program reads input from the console and outputs to the console
    If an input file is specified by -i, it is read instead.
    If an output file is specified by -o, the output is written there.
    To facilitate viewing of the gcode, a gnuplot-readable output file can be specified by -g. It can be viewed by gnuplot and entering: splot output.gnuplot u 1:2:3:4 w lp palette 

WARNING

The outputed gcode needs to be validated by YOU. The software is not perfect, and I am not liable to any harm or damage done by defective gcode!
Example usage:

    ./grecode -xflip tests/testcode.ngc -o tests/out.ngc
    ./grecode -xflip tests/maxboard_cut.ngc -o tests/testcut.ngc
    ./grecode -align cmin cmin tests/maxboard_cut.ngc -o tests/testalign.ngc
    cat tests/testcode.ngc |./grecode -xflip >tests/out2.ngc
    ./grecode tests/max232_kombi.ngc -rot 45 -o tests/test.ngc -g tests/test.xy
    ./grecode tests/arctest.ngc -killn -rot 180 -o tests/test.ngc -g tests/test.xy
    ./grecode tests/abstest.ngc -knive 0.1 -o tests/test.ngc -g tests/test.xy 