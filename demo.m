% A Matlab script for demonstrating the results of inference
% Author: Janne Toivola


data = load('temp.txt');
figure(1);
imagesc(data'); colormap(1-gray);


t = 1:200; % time axis
y = sin(2 * pi * (0.0001*t + 0.005).*t); 
im = [(y>0.33); (y<=0.33 & y>=-0.33); (y<-0.33)]'; % sampling
figure(2);
imagesc(im'); colormap(1-gray);

% save the generated data
fid = fopen('demodata.txt', 'wt');
for i=1:200
  fprintf(fid, '%d, %d, %d\n', im(i,1), im(i,2), im(i,3));
end
fclose(fid);
