@echo off
set /p string= 
set line=%string%

IF "%1"=="'a-z'" goto uppercase

:lowercase
	SET line=%line:A=a%
	SET line=%line:B=b%
	SET line=%line:C=c%
	SET line=%line:D=d%
	SET line=%line:E=e%
	SET line=%line:F=f%
	SET line=%line:G=g%
	SET line=%line:H=h%
	SET line=%line:I=i%
	SET line=%line:J=j%
	SET line=%line:K=k%
	SET line=%line:L=l%
	SET line=%line:M=m%
	SET line=%line:N=n%
	SET line=%line:O=o%
	SET line=%line:P=p%
	SET line=%line:Q=q%
	SET line=%line:R=r%
	SET line=%line:S=s%
	SET line=%line:T=t%
	SET line=%line:U=u%
	SET line=%line:V=v%
	SET line=%line:W=w%
	SET line=%line:X=x%
	SET line=%line:Y=y%
goto exit

:uppercase
	SET line=%line:a=A%
	SET line=%line:b=B%
	SET line=%line:c=C%
	SET line=%line:d=D%
	SET line=%line:e=E%
	SET line=%line:f=F%
	SET line=%line:g=G%
	SET line=%line:h=H%
	SET line=%line:i=I%
	SET line=%line:j=J%
	SET line=%line:k=K%
	SET line=%line:l=L%
	SET line=%line:m=M%
	SET line=%line:n=N%
	SET line=%line:o=O%
	SET line=%line:p=P%
	SET line=%line:q=Q%
	SET line=%line:r=R%
	SET line=%line:s=S%
	SET line=%line:t=T%
	SET line=%line:u=U%
	SET line=%line:v=V%
	SET line=%line:w=W%
	SET line=%line:x=X%
	SET line=%line:y=Y%
	SET line=%line:z=Z%
	

:exit
ECHO %line%

