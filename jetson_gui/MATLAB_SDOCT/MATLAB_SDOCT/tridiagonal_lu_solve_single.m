% clear; close all, clc;fclose('all'); 

% this code is for tridiadonal matrix only
% total n+1 knot points, matrix dimension = N = n+1


function [u] = tridiagonal_lu_solve_single(cnt, lower_diagonal, diagonal, upper_diagonal, RHS, n)
%% for testing
% n=4;
% 
% h_val=single(2);
% h=single(h_val+zeros(1, n));
% x=single([1, 3, 5, 7, 9]);
% y=single([0, 0, 1, 2, 1]);
% 
% diagonal=single([1, 4, 4, 4, 1]);
% diagonal=single(zeros(1, n+1)+1);
% diagonal(2:n)=single(4*h_val);
% lower_diagonal = single(cat(2, h(1:n-1), 0));
% upper_diagonal = single(cat(2, 0, h(2:n)));
% 
% 
% RHS=single(zeros(n+1));
%%
% a=single(zeros(n));
% b=single(zeros(n));
% c=single(zeros(n));
% d=single(zeros(n));
%% for testing
% for i=2:n
%     RHS(i) = single((y(i+1)-y(i))/h(i)  -  (y(i)-y(i-1))/h(i-1));
% end
% RHS=single(RHS*6);
%%




N=n+1;
a=single(zeros(1, N-1));
b=single(zeros(1, N));
c=single(zeros(1, N-1));
a=single(lower_diagonal);
b=single(diagonal);
c=single(upper_diagonal);


l=single(zeros(1, N-1));
v=single(zeros(1, N));

%lu decomposition
v(1)=single(b(1));
for k=2:N
    l(k-1) = single(a(k-1)/v(k-1));
    v(k) = single(b(k) - l(k-1)*c(k-1));
end

%forward
f=single(zeros(1, N));
f=single(RHS);
y=single(zeros(1, N));
y(1)=single(f(1));
for k=2:N
    y(k) = single(f(k) -  l(k-1)*y(k-1));
end

%backward
u=single(zeros(1, N));
u(N)=single(y(N)/v(N));
for k = N-1 : -1 : 1
    u(k)=single((  y(k) - c(k)*u(k+1)  )  /v(k));
end



if (cnt==1)
    % y
    
    filename0="F:\PS_With_MEMS_5kHz_oct_sys_dc_log_wppff.xpr\PS_With_MEMS_5kHz_oct_sys_dc_log_wppff.xpr\project_1\coe_txt\l.txt";
    fd0 = fopen(filename0,'w'); 
    fprintf(fd0,'memory_initialization_radix=16;\n');
    fprintf(fd0,'memory_initialization_vector=\n');
    for k=1:2047
        if(k~=2048)
            fprintf(fd0,'%8x,\n', typecast(l(k), 'uint32') );
        else
            fprintf(fd0,'%8x;', typecast(l(k), 'uint32') );

        end
    end


    filename1="F:\PS_With_MEMS_5kHz_oct_sys_dc_log_wppff.xpr\PS_With_MEMS_5kHz_oct_sys_dc_log_wppff.xpr\project_1\coe_txt\v.txt";
    fd1 = fopen(filename1,'w'); 
    fprintf(fd1,'memory_initialization_radix=16;\n');
    fprintf(fd1,'memory_initialization_vector=\n');
    for k=1:2048
        if(k~=2048)
            fprintf(fd1,'%8x,\n', typecast(v(k), 'uint32') );
        else
            fprintf(fd1,'%8x;', typecast(v(k), 'uint32') );

        end
    end


    filename2="F:\PS_With_MEMS_5kHz_oct_sys_dc_log_wppff.xpr\PS_With_MEMS_5kHz_oct_sys_dc_log_wppff.xpr\project_1\coe_txt\c.txt";
    fd2 = fopen(filename2,'w'); 
    fprintf(fd2,'memory_initialization_radix=16;\n');
    fprintf(fd2,'memory_initialization_vector=\n');
    for k=1:2047
        if(k~=2048)
            fprintf(fd2,'%8x,\n', typecast(c(k), 'uint32') );
        else
            fprintf(fd2,'%8x;', typecast(c(k), 'uint32') );

        end
    end
end


end



