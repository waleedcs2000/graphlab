%%
fnname = 'test';
%% run emlc
cfg = emlcoder.CompilerOptions;
cfg.DynamicMemoryAllocation = 'AllVariableSizeArrays';
cfg.EnableVariableSizing = true;
%eginput = 0;
eginput.a = emlcoder.egs(0,[Inf,Inf]);
temp.a = 10;
temp.b = 20;
%temp.c = 'hello world';
temp.c = emlcoder.egs('a',[Inf]);
eginput.b = emlcoder.egs(temp,[Inf]);

emlcstring= ['emlc -c -T RTW -d b2 ' ...
             fnname ' -eg {eginput} ' ...
             '-s cfg'];
disp(emlcstring);
eval(emlcstring);
%% generate
[stat,res] = system('python mxarray_to_emlc.py b2/test_types.h > b2/generator.hpp');
if (~isempty(res))
    disp 'Compilation Failed. ';
    disp(res);
    return;
end
%% compile
% get all the generated c files
allcfiles = dir('b2/*.c');
str = '';
for i = 1:length(allcfiles)
    str = [str 'b2/' allcfiles(i).name ' '];
end
%%
compilestring = ['mex -g '  ...
  '-I. -I/usr/include ' ...
 '-Ib2/ ' ...
 '-cxx ' ...
 'CC="g++" ' ...
 '-DBOOST_UBLAS_ENABLE_PROXY_SHORTCUTS ' ...
 '-D_SCL_SECURE_NO_WARNINGS ' ...
 '-D_CRT_SECURE_NO_WARNINGS ' ...
 '-D_SECURE_SCL=0 ' ...
 '-DMEX_COMPILE ' ...   
 'CXXFLAGS="$CXXFLAGS -g -fpic" ' ...
 '-output test_mex ' ...
 'test_mex.cpp ' ...
 str ];

disp(compilestring);
%%
eval(compilestring);