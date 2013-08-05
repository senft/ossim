#!/bin/sh
echo "\textbf{forw. in 1+} & "
grep "forwardingInMoreThanOne" 1/*.sca | awk '{print $4}' | awk 'NR == 1 { max=$1; min=$1; sum=0 }
   { if ($1>max) max=$1; if ($1<min) min=$1; sum+=$1;}
   END {printf "%.3f (%.3f, %.3f) & ", sum/NR, min, max}'
grep "forwardingInMoreThanOne" 4/*.sca | awk '{print $4}' | awk 'NR == 1 { max=$1; min=$1; sum=0 }
   { if ($1>max) max=$1; if ($1<min) min=$1; sum+=$1;}
   END {printf "%.3f (%.3f, %.3f) & ", sum/NR, min, max}'
grep "forwardingInMoreThanOne" 8/*.sca | awk '{print $4}' | awk 'NR == 1 { max=$1; min=$1; sum=0 }
   { if ($1>max) max=$1; if ($1<min) min=$1; sum+=$1;}
   END {printf "%.3f (%.3f, %.3f) & ", sum/NR, min, max}'
grep "forwardingInMoreThanOne" 16/*.sca | awk '{print $4}' | awk 'NR == 1 { max=$1; min=$1; sum=0 }
   { if ($1>max) max=$1; if ($1<min) min=$1; sum+=$1;}
   END {printf "%.3f (%.3f, %.3f) \\\\ ", sum/NR, min, max}'


echo
echo "\hline"
echo "\textbf{forw. in 1} &"
grep "forwardingInOne" 1/*.sca | awk '{print $4}' | awk 'NR == 1 { max=$1; min=$1; sum=0 }
   { if ($1>max) max=$1; if ($1<min) min=$1; sum+=$1;}
   END {printf "%.3f (%.3f, %.3f) & ", sum/NR, min, max}'
grep "forwardingInOne" 4/*.sca | awk '{print $4}' | awk 'NR == 1 { max=$1; min=$1; sum=0 }
   { if ($1>max) max=$1; if ($1<min) min=$1; sum+=$1;}
   END {printf "%.3f (%.3f, %.3f) & ", sum/NR, min, max}'
grep "forwardingInOne" 8/*.sca | awk '{print $4}' | awk 'NR == 1 { max=$1; min=$1; sum=0 }
   { if ($1>max) max=$1; if ($1<min) min=$1; sum+=$1;}
   END {printf "%.3f (%.3f, %.3f) & ", sum/NR, min, max}'
grep "forwardingInOne" 16/*.sca | awk '{print $4}' | awk 'NR == 1 { max=$1; min=$1; sum=0 }
   { if ($1>max) max=$1; if ($1<min) min=$1; sum+=$1;}
   END {printf "%.3f (%.3f, %.3f) \\\\ ", sum/NR, min, max}'


echo
echo "\hline"
echo "\textbf{forw. in 0} &"
grep "forwardingInNone" 1/*.sca | awk '{print $4}' | awk 'NR == 1 { max=$1; min=$1; sum=0 }
   { if ($1>max) max=$1; if ($1<min) min=$1; sum+=$1;}
   END {printf "%.3f (%.3f, %.3f) & ", sum/NR, min, max}'
grep "forwardingInNone" 4/*.sca | awk '{print $4}' | awk 'NR == 1 { max=$1; min=$1; sum=0 }
   { if ($1>max) max=$1; if ($1<min) min=$1; sum+=$1;}
   END {printf "%.3f (%.3f, %.3f) & ", sum/NR, min, max}'
grep "forwardingInNone" 8/*.sca | awk '{print $4}' | awk 'NR == 1 { max=$1; min=$1; sum=0 }
   { if ($1>max) max=$1; if ($1<min) min=$1; sum+=$1;}
   END {printf "%.3f (%.3f, %.3f) & ", sum/NR, min, max}'
grep "forwardingInNone" 16/*.sca | awk '{print $4}' | awk 'NR == 1 { max=$1; min=$1; sum=0 }
   { if ($1>max) max=$1; if ($1<min) min=$1; sum+=$1;}
   END {printf "%.3f (%.3f, %.3f) \\\\ ", sum/NR, min, max}'


echo
echo "\hline"
echo "\textbf{mean number of trees nodes forward in} &"
grep "meanNumTrees" 1/*.sca | awk '{print $4}' | awk 'NR == 1 { max=$1; min=$1; sum=0 }
   { if ($1>max) max=$1; if ($1<min) min=$1; sum+=$1;}
   END {printf "%.3f (%.3f, %.3f) & ", sum/NR, min, max}'
grep "meanNumTrees" 4/*.sca | awk '{print $4}' | awk 'NR == 1 { max=$1; min=$1; sum=0 }
   { if ($1>max) max=$1; if ($1<min) min=$1; sum+=$1;}
   END {printf "%.3f (%.3f, %.3f) & ", sum/NR, min, max}'
grep "meanNumTrees" 8/*.sca | awk '{print $4}' | awk 'NR == 1 { max=$1; min=$1; sum=0 }
   { if ($1>max) max=$1; if ($1<min) min=$1; sum+=$1;}
   END {printf "%.3f (%.3f, %.3f) & ", sum/NR, min, max}'
grep "meanNumTrees" 16/*.sca | awk '{print $4}' | awk 'NR == 1 { max=$1; min=$1; sum=0 }
   { if ($1>max) max=$1; if ($1<min) min=$1; sum+=$1;}
   END {printf "%.3f (%.3f, %.3f) \\\\ ", sum/NR, min, max}'


echo
echo "\hline"
echo "\textbf{mean tree height} &"
grep "meanTreeHeight" 1/*.sca | awk '{print $4}' | awk 'NR == 1 { max=$1; min=$1; sum=0 }
   { if ($1>max) max=$1; if ($1<min) min=$1; sum+=$1;}
   END {printf "%.3f (%.3f, %.3f) & ", sum/NR, min, max}'
grep "meanTreeHeight" 4/*.sca | awk '{print $4}' | awk 'NR == 1 { max=$1; min=$1; sum=0 }
   { if ($1>max) max=$1; if ($1<min) min=$1; sum+=$1;}
   END {printf "%.3f (%.3f, %.3f) & ", sum/NR, min, max}'
grep "meanTreeHeight" 8/*.sca | awk '{print $4}' | awk 'NR == 1 { max=$1; min=$1; sum=0 }
   { if ($1>max) max=$1; if ($1<min) min=$1; sum+=$1;}
   END {printf "%.3f (%.3f, %.3f) & ", sum/NR, min, max}'
grep "meanTreeHeight" 16/*.sca | awk '{print $4}' | awk 'NR == 1 { max=$1; min=$1; sum=0 }
   { if ($1>max) max=$1; if ($1<min) min=$1; sum+=$1;}
   END {printf "%.3f (%.3f, %.3f) \\\\ ", sum/NR, min, max}'


echo
echo "\hline"
echo "\textbf{mean retrys} &"
grep "meanRetrys" 1/*.sca | awk '{print $4}' | awk 'NR == 1 { max=$1; min=$1; sum=0 }
   { if ($1>max) max=$1; if ($1<min) min=$1; sum+=$1;}
   END {printf "%.3f (%.3f, %.3f) & ", sum/NR, min, max}'
grep "meanRetrys" 4/*.sca | awk '{print $4}' | awk 'NR == 1 { max=$1; min=$1; sum=0 }
   { if ($1>max) max=$1; if ($1<min) min=$1; sum+=$1;}
   END {printf "%.3f (%.3f, %.3f) & ", sum/NR, min, max}'
grep "meanRetrys" 8/*.sca | awk '{print $4}' | awk 'NR == 1 { max=$1; min=$1; sum=0 }
   { if ($1>max) max=$1; if ($1<min) min=$1; sum+=$1;}
   END {printf "%.3f (%.3f, %.3f) & ", sum/NR, min, max}'
grep "meanRetrys" 16/*.sca | awk '{print $4}' | awk 'NR == 1 { max=$1; min=$1; sum=0 }
   { if ($1>max) max=$1; if ($1<min) min=$1; sum+=$1;}
   END {printf "%.3f (%.3f, %.3f) \\\\ ", sum/NR, min, max}'


echo
echo "\hline"
echo "\textbf{total retrys} &"
grep "totalRetrys" 1/*.sca | awk '{print $4}' | awk 'NR == 1 { max=$1; min=$1; sum=0 }
   { if ($1>max) max=$1; if ($1<min) min=$1; sum+=$1;}
   END {printf "%.3f (%.3f, %.3f) & ", sum/NR, min, max}'
grep "totalRetrys" 4/*.sca | awk '{print $4}' | awk 'NR == 1 { max=$1; min=$1; sum=0 }
   { if ($1>max) max=$1; if ($1<min) min=$1; sum+=$1;}
   END {printf "%.3f (%.3f, %.3f) & ", sum/NR, min, max}'
grep "totalRetrys" 8/*.sca | awk '{print $4}' | awk 'NR == 1 { max=$1; min=$1; sum=0 }
   { if ($1>max) max=$1; if ($1<min) min=$1; sum+=$1;}
   END {printf "%.3f (%.3f, %.3f) & ", sum/NR, min, max}'
grep "totalRetrys" 16/*.sca | awk '{print $4}' | awk 'NR == 1 { max=$1; min=$1; sum=0 }
   { if ($1>max) max=$1; if ($1<min) min=$1; sum+=$1;}
   END {printf "%.3f (%.3f, %.3f) \\\\ ", sum/NR, min, max}'


echo
echo "\hline"
echo "\textbf{max retrys} &"
grep "maxRetrys" 1/*.sca | awk '{print $4}' | awk 'NR == 1 { max=$1; min=$1; sum=0 }
   { if ($1>max) max=$1; if ($1<min) min=$1; sum+=$1;}
   END {printf "%.3f (%.3f, %.3f) & ", sum/NR, min, max}'
grep "maxRetrys" 4/*.sca | awk '{print $4}' | awk 'NR == 1 { max=$1; min=$1; sum=0 }
   { if ($1>max) max=$1; if ($1<min) min=$1; sum+=$1;}
   END {printf "%.3f (%.3f, %.3f) & ", sum/NR, min, max}'
grep "maxRetrys" 8/*.sca | awk '{print $4}' | awk 'NR == 1 { max=$1; min=$1; sum=0 }
   { if ($1>max) max=$1; if ($1<min) min=$1; sum+=$1;}
   END {printf "%.3f (%.3f, %.3f) & ", sum/NR, min, max}'
grep "maxRetrys" 16/*.sca | awk '{print $4}' | awk 'NR == 1 { max=$1; min=$1; sum=0 }
   { if ($1>max) max=$1; if ($1<min) min=$1; sum+=$1;}
   END {printf "%.3f (%.3f, %.3f) \\\\ ", sum/NR, min, max}'


echo
echo "\hline"
echo "\textbf{mean bandwidth utilization} & "
grep "bwUtil" 1/*.sca | awk '{print $4}' | awk 'NR == 1 { max=$1; min=$1; sum=0 }
   { if ($1>max) max=$1; if ($1<min) min=$1; sum+=$1;}
   END {printf "%.3f (%.3f, %.3f) & ", sum/NR, min, max}'
grep "bwUtil" 4/*.sca | awk '{print $4}' | awk 'NR == 1 { max=$1; min=$1; sum=0 }
   { if ($1>max) max=$1; if ($1<min) min=$1; sum+=$1;}
   END {printf "%.3f (%.3f, %.3f) & ", sum/NR, min, max}'
grep "bwUtil" 8/*.sca | awk '{print $4}' | awk 'NR == 1 { max=$1; min=$1; sum=0 }
   { if ($1>max) max=$1; if ($1<min) min=$1; sum+=$1;}
   END {printf "%.3f (%.3f, %.3f) & ", sum/NR, min, max}'
grep "bwUtil" 16/*.sca | awk '{print $4}' | awk 'NR == 1 { max=$1; min=$1; sum=0 }
   { if ($1>max) max=$1; if ($1<min) min=$1; sum+=$1;}
   END {printf "%.3f (%.3f, %.3f) \\\\ ", sum/NR, min, max}'


echo
echo "\hline"
echo "\textbf{message count} & "
grep "messageCount:" 1/*.sca | awk '{print $4}' | awk 'NR == 1 { max=$1; min=$1; sum=0 }
   { if ($1>max) max=$1; if ($1<min) min=$1; sum+=$1;}
   END {printf "%.3f (%.3f, %.3f) & ", sum/NR, min, max}'
grep "messageCount:" 4/*.sca | awk '{print $4}' | awk 'NR == 1 { max=$1; min=$1; sum=0 }
   { if ($1>max) max=$1; if ($1<min) min=$1; sum+=$1;}
   END {printf "%.3f (%.3f, %.3f) & ", sum/NR, min, max}'
grep "messageCount:" 8/*.sca | awk '{print $4}' | awk 'NR == 1 { max=$1; min=$1; sum=0 }
   { if ($1>max) max=$1; if ($1<min) min=$1; sum+=$1;}
   END {printf "%.3f (%.3f, %.3f) & ", sum/NR, min, max}'
grep "messageCount:" 16/*.sca | awk '{print $4}' | awk 'NR == 1 { max=$1; min=$1; sum=0 }
   { if ($1>max) max=$1; if ($1<min) min=$1; sum+=$1;}
   END {printf "%.3f (%.3f, %.3f) \\\\ ", sum/NR, min, max}'


echo
echo "\hline"
echo "\textbf{message count: connect request} & "
grep "messageCountCR" 1/*.sca | awk '{print $4}' | awk 'NR == 1 { max=$1; min=$1; sum=0 }
   { if ($1>max) max=$1; if ($1<min) min=$1; sum+=$1;}
   END {printf "%.3f (%.3f, %.3f) & ", sum/NR, min, max}'
grep "messageCountCR" 4/*.sca | awk '{print $4}' | awk 'NR == 1 { max=$1; min=$1; sum=0 }
   { if ($1>max) max=$1; if ($1<min) min=$1; sum+=$1;}
   END {printf "%.3f (%.3f, %.3f) & ", sum/NR, min, max}'
grep "messageCountCR" 8/*.sca | awk '{print $4}' | awk 'NR == 1 { max=$1; min=$1; sum=0 }
   { if ($1>max) max=$1; if ($1<min) min=$1; sum+=$1;}
   END {printf "%.3f (%.3f, %.3f) & ", sum/NR, min, max}'
grep "messageCountCR" 16/*.sca | awk '{print $4}' | awk 'NR == 1 { max=$1; min=$1; sum=0 }
   { if ($1>max) max=$1; if ($1<min) min=$1; sum+=$1;}
   END {printf "%.3f (%.3f, %.3f) \\\\ ", sum/NR, min, max}'


echo
echo "\hline"
echo "\textbf{message count: disconnect request} & "
grep "messageCountDR" 1/*.sca | awk '{print $4}' | awk 'NR == 1 { max=$1; min=$1; sum=0 }
   { if ($1>max) max=$1; if ($1<min) min=$1; sum+=$1;}
   END {printf "%.3f (%.3f, %.3f) & ", sum/NR, min, max}'
grep "messageCountDR" 4/*.sca | awk '{print $4}' | awk 'NR == 1 { max=$1; min=$1; sum=0 }
   { if ($1>max) max=$1; if ($1<min) min=$1; sum+=$1;}
   END {printf "%.3f (%.3f, %.3f) & ", sum/NR, min, max}'
grep "messageCountDR" 8/*.sca | awk '{print $4}' | awk 'NR == 1 { max=$1; min=$1; sum=0 }
   { if ($1>max) max=$1; if ($1<min) min=$1; sum+=$1;}
   END {printf "%.3f (%.3f, %.3f) & ", sum/NR, min, max}'
grep "messageCountDR" 16/*.sca | awk '{print $4}' | awk 'NR == 1 { max=$1; min=$1; sum=0 }
   { if ($1>max) max=$1; if ($1<min) min=$1; sum+=$1;}
   END {printf "%.3f (%.3f, %.3f) \\\\ ", sum/NR, min, max}'


echo
echo "\hline"
echo "\textbf{message count: connect confirm} & "
grep "messageCountCC" 1/*.sca | awk '{print $4}' | awk 'NR == 1 { max=$1; min=$1; sum=0 }
   { if ($1>max) max=$1; if ($1<min) min=$1; sum+=$1;}
   END {printf "%.3f (%.3f, %.3f) & ", sum/NR, min, max}'
grep "messageCountCC" 4/*.sca | awk '{print $4}' | awk 'NR == 1 { max=$1; min=$1; sum=0 }
   { if ($1>max) max=$1; if ($1<min) min=$1; sum+=$1;}
   END {printf "%.3f (%.3f, %.3f) & ", sum/NR, min, max}'
grep "messageCountCC" 8/*.sca | awk '{print $4}' | awk 'NR == 1 { max=$1; min=$1; sum=0 }
   { if ($1>max) max=$1; if ($1<min) min=$1; sum+=$1;}
   END {printf "%.3f (%.3f, %.3f) & ", sum/NR, min, max}'
grep "messageCountCC" 16/*.sca | awk '{print $4}' | awk 'NR == 1 { max=$1; min=$1; sum=0 }
   { if ($1>max) max=$1; if ($1<min) min=$1; sum+=$1;}
   END {printf "%.3f (%.3f, %.3f) \\\\ ", sum/NR, min, max}'


echo
echo "\hline"
echo "\textbf{message count: pass node request} & "
grep "messageCountPNR" 1/*.sca | awk '{print $4}' | awk 'NR == 1 { max=$1; min=$1; sum=0 }
   { if ($1>max) max=$1; if ($1<min) min=$1; sum+=$1;}
   END {printf "%.3f (%.3f, %.3f) & ", sum/NR, min, max}'
grep "messageCountPNR" 4/*.sca | awk '{print $4}' | awk 'NR == 1 { max=$1; min=$1; sum=0 }
   { if ($1>max) max=$1; if ($1<min) min=$1; sum+=$1;}
   END {printf "%.3f (%.3f, %.3f) & ", sum/NR, min, max}'
grep "messageCountPNR" 8/*.sca | awk '{print $4}' | awk 'NR == 1 { max=$1; min=$1; sum=0 }
   { if ($1>max) max=$1; if ($1<min) min=$1; sum+=$1;}
   END {printf "%.3f (%.3f, %.3f) & ", sum/NR, min, max}'
grep "messageCountPNR" 16/*.sca | awk '{print $4}' | awk 'NR == 1 { max=$1; min=$1; sum=0 }
   { if ($1>max) max=$1; if ($1<min) min=$1; sum+=$1;}
   END {printf "%.3f (%.3f, %.3f) \\\\ ", sum/NR, min, max}'


echo
echo "\hline"
echo "\textbf{message count: successor info} & "
grep "messageCountSI" 1/*.sca | awk '{print $4}' | awk 'NR == 1 { max=$1; min=$1; sum=0 }
   { if ($1>max) max=$1; if ($1<min) min=$1; sum+=$1;}
   END {printf "%.3f (%.3f, %.3f) & ", sum/NR, min, max}'
grep "messageCountSI" 4/*.sca | awk '{print $4}' | awk 'NR == 1 { max=$1; min=$1; sum=0 }
   { if ($1>max) max=$1; if ($1<min) min=$1; sum+=$1;}
   END {printf "%.3f (%.3f, %.3f) & ", sum/NR, min, max}'
grep "messageCountSI" 8/*.sca | awk '{print $4}' | awk 'NR == 1 { max=$1; min=$1; sum=0 }
   { if ($1>max) max=$1; if ($1<min) min=$1; sum+=$1;}
   END {printf "%.3f (%.3f, %.3f) & ", sum/NR, min, max}'
grep "messageCountSI" 16/*.sca | awk '{print $4}' | awk 'NR == 1 { max=$1; min=$1; sum=0 }
   { if ($1>max) max=$1; if ($1<min) min=$1; sum+=$1;}
   END {printf "%.3f (%.3f, %.3f) \\\\ ", sum/NR, min, max}'



echo
