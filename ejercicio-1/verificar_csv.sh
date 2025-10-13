#!/bin/bash
archivo=${1:-datos_generados.csv}

sort -t, -k1,1n "$archivo" | awk -F, '
NR==1{print; prev=0; next}
NR>1{
  id=$1+0;
  if (id==prev) dup++;
  if (prev>0 && id!=prev+1) gap++;
  prev=id
}
END{
  if (dup>0) print "❌ ERROR: Duplicados:",dup; else print "✅ OK: Sin duplicados";
  if (gap>0) print "❌ ERROR: Saltos:",gap; else print "✅ OK: Sin saltos";
}'