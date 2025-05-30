-- Allow SELECT
create policy allow_select_on_lectura
on lectura
for select
to authenticated, anon
using (true);

-- Allow INSERT
create policy allow_insert_on_lectura
on lectura
for insert
to authenticated, anon
with check (true);
