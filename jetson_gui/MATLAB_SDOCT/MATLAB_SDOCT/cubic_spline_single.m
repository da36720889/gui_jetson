%% for testing
% clear; close all, clc;fclose('all'); 
% 
% % total n+1 knot points, from x0, y0 to xn, yn
% h_val=1.6;
% x=1:h_val:6;
% 
% S=size(x);
% n=S(2)-1;
% 
% y=zeros(1, n+1);
% y(1:2)=[2 1];
% 
% 
% 
% 
% x_interp=cat(2, 1 : 0.1 : x(end)-0.01);
%%
function [y_interp] = cubic_spline_single(cnt, x, y, x_interp)

% h_val=single(0.00048852);
h_val=single(4.885197850512946e-4);
S=size(x);
n=S(2)-1;






h=single(h_val+zeros(1, n));

diagonal=single(zeros(1, n+1)+1);
diagonal(2:n)=single(4*h_val);
lower_diagonal = single(cat(2, h(1:n-1), 0));
upper_diagonal = single(cat(2, 0, h(2:n)));


RHS=single(zeros(1, n+1));


a=single(zeros(1, n));
b=single(zeros(1, n));
c=single(zeros(1, n));
d=single(zeros(1, n));

for i=2:n
    RHS(i) = single((y(i+1)-y(i))/h(i)  -  (y(i)-y(i-1))/h(i-1));
end
RHS=single(RHS*6);
% if(cnt==1)
%     RHS
% end

A=single(diag(diagonal) + diag(lower_diagonal, -1) + diag(upper_diagonal, 1));
m=single(tridiagonal_lu_solve_single(cnt, lower_diagonal, diagonal, upper_diagonal, (RHS'), n));



a=single(y);
for i=1:n
    b(i) = single((y(i+1)-y(i))/h(i)  -  h(i)/2*m(i)  -  h(i)/6*(m(i+1)-m(i)));
end
c=single(m/2);
for i=1:n
    d(i) = single((m(i+1)-m(i))  /6/h(i));
end



s_index=single(floor( (x_interp-x(1) )  /  h_val ) +1 );
s_index(end)=single(2047);
X=single(x_interp-x(s_index));
X(end)=single(1-x(end-1));

if(cnt==1)
    s_index
    X
end


S=size(x_interp);
for i=1:S(2)
    
    y_interp(i)=single(a(s_index(i)) + b(s_index(i))*X(i) + c(s_index(i))*X(i)^2 + d(s_index(i))*X(i)^3);
end



% filename="./lower_diagonal.txt";
% fd = fopen(filename,'w');  %#ok<MFAMB>
% for k=1:n
%     fprintf(fd,'%8x\n', typecast(lower_diagonal(k), 'uint32') );
% end
% 
% filename="./lower_diagonal.txt";
% fprintf(fd,'memory_initialization_radix=16;\n');
% fprintf(fd,'memory_initialization_vector=\n');







if (cnt==2)
        filename4="F:\PS_With_MEMS_5kHz_oct_sys_dc_log_wppff.xpr\PS_With_MEMS_5kHz_oct_sys_dc_log_wppff.xpr\project_1\coe_txt\y_bg_2.txt";
        fd4 = fopen(filename4,'w'); 

        for k=1:2048
            fprintf(fd4,'%8x\n',  typecast(y(k), 'uint32') );

        end

        filename6="F:\PS_With_MEMS_5kHz_oct_sys_dc_log_wppff.xpr\PS_With_MEMS_5kHz_oct_sys_dc_log_wppff.xpr\project_1\coe_txt\interp_data_2.txt";
        fd6 = fopen(filename6,'w'); 

        for k=1:2048
            fprintf(fd6,'%f\n',  y_interp(k) );

        end
end




if (cnt==1)

    % a
    % b
    % c
    % d
    % y_interp
    % RHS
    % m




    filename0="F:\PS_With_MEMS_5kHz_oct_sys_dc_log_wppff.xpr\PS_With_MEMS_5kHz_oct_sys_dc_log_wppff.xpr\project_1\coe_txt\X.txt";
    fd0 = fopen(filename0,'w'); 
    fprintf(fd0,'memory_initialization_radix=16;\n');
    fprintf(fd0,'memory_initialization_vector=\n');
    for k=1:2048
        if(k~=2048)
            fprintf(fd0,'%8x,\n', typecast(X(k), 'uint32') );
        else
            fprintf(fd0,'%8x;', typecast(X(k), 'uint32') );

        end
    end



    filename1="F:\PS_With_MEMS_5kHz_oct_sys_dc_log_wppff.xpr\PS_With_MEMS_5kHz_oct_sys_dc_log_wppff.xpr\project_1\coe_txt\X_2.txt";
    fd1 = fopen(filename1,'w'); 
    fprintf(fd1,'memory_initialization_radix=16;\n');
    fprintf(fd1,'memory_initialization_vector=\n');
    for k=1:2048
        if(k~=2048)
            fprintf(fd1,'%8x,\n', typecast((X(k))^2, 'uint32') );
        else
            fprintf(fd1,'%8x;', typecast((X(k))^2, 'uint32') );

        end
    end


    filename2="F:\PS_With_MEMS_5kHz_oct_sys_dc_log_wppff.xpr\PS_With_MEMS_5kHz_oct_sys_dc_log_wppff.xpr\project_1\coe_txt\X_3.txt";
    fd2 = fopen(filename2,'w'); 
    fprintf(fd2,'memory_initialization_radix=16;\n');
    fprintf(fd2,'memory_initialization_vector=\n');
    for k=1:2048
        if(k~=2048)
            fprintf(fd2,'%8x,\n', typecast((X(k))^3, 'uint32') );
        else
            fprintf(fd2,'%8x;', typecast((X(k))^3, 'uint32') );

        end
    end



    filename3="F:\PS_With_MEMS_5kHz_oct_sys_dc_log_wppff.xpr\PS_With_MEMS_5kHz_oct_sys_dc_log_wppff.xpr\project_1\coe_txt\spline_index.txt";
    fd3 = fopen(filename3,'w'); 
    fprintf(fd3,'memory_initialization_radix=16;\n');
    fprintf(fd3,'memory_initialization_vector=\n');
    for k=1:2048
        if(k~=2048)
            fprintf(fd3,'%3x,\n', s_index(k)-1 );
        else
            fprintf(fd3,'%3x;', s_index(k)-1 );

        end
    end



    filename4="F:\PS_With_MEMS_5kHz_oct_sys_dc_log_wppff.xpr\PS_With_MEMS_5kHz_oct_sys_dc_log_wppff.xpr\project_1\coe_txt\y_bg.txt";
    fd4 = fopen(filename4,'w'); 

    for k=1:2048
        fprintf(fd4,'%8x\n',  typecast(y(k), 'uint32') );

    end




    filename5="F:\PS_With_MEMS_5kHz_oct_sys_dc_log_wppff.xpr\PS_With_MEMS_5kHz_oct_sys_dc_log_wppff.xpr\project_1\coe_txt\coeff_abcd.txt";
    fd5 = fopen(filename5,'w'); 

    for k=1:2047
        fprintf(fd5,'%8x\n',  typecast(a(k), 'uint32') );

    end
    for k=1:2047
        fprintf(fd5,'%8x\n',  typecast(b(k), 'uint32') );

    end
    for k=1:2047
        fprintf(fd5,'%8x\n',  typecast(c(k), 'uint32') );

    end
    for k=1:2047
        fprintf(fd5,'%8x\n',  typecast(d(k), 'uint32') );

    end


    filename6="F:\PS_With_MEMS_5kHz_oct_sys_dc_log_wppff.xpr\PS_With_MEMS_5kHz_oct_sys_dc_log_wppff.xpr\project_1\coe_txt\interp_data.txt";
    fd6 = fopen(filename6,'w'); 

    for k=1:2048
        fprintf(fd6,'%f\n',  y_interp(k) );

    end







end






%% for testing

% figure
% plot(x_interp, y_interp);
% xlim([x(1), x(n+1)]);
% ylim([min(y_interp), max(y_interp)]);
% 
% 
% figure
% plot(x, y);
% xlim([x(1), x(n+1)]);
% ylim([min(y_interp), max(y_interp)]);
%%
end

