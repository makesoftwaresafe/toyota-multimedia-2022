tdata <- scan(file1, list(day=0, sec=0, m=0, w=0, x=0, y=0, z=0))
str <- paste("eps/", file1, ".eps", sep="")
postscript(str,  ,  ,  , 5, pointsize=18)
par(mgp=c(1, 0, 0), tck=0.03, mar=c(2, 2, 1, 1))
plot(tdata$sec, tdata$m, type="l", xlab=paste("MJD", tdata$day, "Time (s)"), ylab="LORAN-M SNR (dB)")

#ifdef __QNXNTO__
#ifdef __USESRCVERSION
.section .ident, "SM",%progbits,1;
.asciz "$URL: http://svn.ott.qnx.com/product/tags/7.0.0/GA/lib/io-pkt/dist/ntp/scripts/stats/tdata.S $ $Rev: 806186 $";
.previous
#endif
#endif
