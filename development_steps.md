# Development Steps

## Necessary Commands

### `create_district(dstr_ID)`

Creates a new directory (district) with the given district ID.

Also:
- keeps track of the one and only manager for this district
- adds the required permissions
- adds the files:
  - `reports.dat`
  - `district.cfg`
  - `logged_district`
- automatically creates the necessary symbolic link:
  - `active_reports-<dstr_ID>`

Alternatively, this functionality can be modified/appended to the `add` command with an optional command-line argument such as `-d` or `--type district` to specify that you want to add a district, not a report.

---

### `add <dstr_ID>`

Append a new report.

- Both roles may add reports.
- The inspector name field is taken from the `--user` argument.
- Set permissions on `reports.dat` to `664` after creation if not already set.

---

### `list <district_id>`

List all reports in a district, and the report file information in the format mentioned above.

---

### `view <district_id> <report_id>`

Print the full details of a specific report.

Available to both roles.

---

### `remove_report <district_id> <report_id>`

Remove a single report.

- Manager role only.
- Use `lseek()` to shift subsequent records one position earlier.
- Then truncate the file with `ftruncate()`.

---

### `update_threshold <district_id> <value>`

Update the severity threshold in `district.cfg`.

- Manager role only.
- Before writing, call `stat()` on `district.cfg`.
- Verify the permission bits match `640`.
- If someone has changed them, refuse and print a diagnostic.

---

### `filter <district_id> <condition>`

Filter and display reports matching a condition.

See the section below.

---

## Optional Functionality

- Add `-h` and/or `--help` functionality to each command.
- Add a function/command to scan for dangling symbolic links and offer to remove them with `Y/N` or `y/n`.