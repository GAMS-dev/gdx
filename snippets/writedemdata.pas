program writedemdata;
uses
   gxfile,
   gmsspecs,
   gxdefs
;

var
  pgx: TGXFileObj;
  msg: ShortString;
  ErrNr: Integer;
  keys: TgdxStrIndex;
  values: TgdxValues;

procedure writeRec(key: ShortString; val: Double);
begin
  keys[1] := key;
  values[vallevel] := val;
  pgx.gdxDataWriteStr(keys, values);
end;

procedure testWriteGdx();
begin;
  pgx := TGXFileObj.Create(msg);
  pgx.gdxOpenWrite('mytest.gdx', 'TGXFileObj', ErrNr);
  pgx.gdxDataWriteStrStart('Demand', 'Demand data', 1, GMS_DT_PAR, 0);
  writeRec('New-York', 324.0);
  writeRec('Chicago', 299.0);
  writeRec('Topeka', 274.0);
  pgx.gdxDataWriteDone;
  pgx.gdxClose;
  pgx.Free;
end;

procedure testReadGdx();
var
  NrRecs, DimFrst, i: Integer;
begin;
  pgx := TGXFileObj.Create(msg);
  pgx.gdxOpenRead('mytest.gdx', ErrNr);
  pgx.gdxDataReadStrStart(1, NrRecs);
  for i := 1 to NrRecs do begin
    pgx.gdxDataReadStr(keys, values, DimFrst);
    WriteLn('Key = ', keys[1], ' Value=', values[vallevel], ' DimFrst=', DimFrst);
  end;
  pgx.gdxClose;
  pgx.Free;
end;

begin
  testWriteGdx();
  testReadGdx();
end.