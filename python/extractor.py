from file_read_backwards import FileReadBackwards

def extract_total(fname):
    with FileReadBackwards(fname) as f:
        for line in f:
            line = line.lower()
            if line.startswith("total"):
                parts = line.split()
                for p in parts:
                    # remove trailing 's' or other non-digit characters
                    p = p.rstrip('s')
                    try:
                        return float(p)   # cast to float
                    except ValueError:
                        continue
    return None
