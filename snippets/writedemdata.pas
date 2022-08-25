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

begin
pgx := TGXFileObj.Create(msg);
pgx.gdxOpenWrite('mytest.gdx', 'TGXFileObj', ErrNr);
pgx.gdxDataWriteStrStart('Demand', 'Demand data', 1, GMS_DT_PAR, 0);
writeRec('New-York', 324.0);
writeRec('Chicago', 299.0);
writeRec('Topeka', 274.0);
pgx.gdxDataWriteDone;
pgx.gdxClose;
pgx.Free;
end.