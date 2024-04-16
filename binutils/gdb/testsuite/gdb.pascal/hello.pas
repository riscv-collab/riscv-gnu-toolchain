program hello;

var
  st : string;

procedure print_hello;
begin
 Writeln('Before assignment'); { set breakpoint 1 here }
 st:='Hello, world!'; 
 writeln(st); {set breakpoint 2 here }
end;

begin
  print_hello;
end. 
