#! /bin/bash

swcfolder=$1


cd $swcfolder
for i in `ls *auto.swc`
do 
	img=` echo ${i:0:16}*.v3draw `
	#img=`echo ${i%_*}.v3draw `
	
	vaa3d -x ML_get_sample -f get_2D_block -i $i -p $img 2 -o $swcfolder
done

for i in `ls *manual.swc`
do 
	img=` echo ${i:0:16}*.v3draw `
	#img=` echo ${i%_*}.v3draw` 
	vaa3d -x ML_get_sample -f get_2D_block -i $i -p $img 1 -o $swcfolder
done



