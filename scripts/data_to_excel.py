import pandas as pd

row_offset = 3
col_offset = 8

def plot_speedup_time(dataset_dir, mode, threads, thread_time, thread_speedup, output_file):

    df = pd.DataFrame({"Threads": threads, "Time (s)": thread_time, "Speedup": thread_speedup})

    with pd.ExcelWriter(output_file, engine="xlsxwriter") as writer:
        df.to_excel(writer, sheet_name="Data", index=False, startrow=row_offset, startcol=col_offset)

        workbook  = writer.book
        worksheet = writer.sheets["Data"]

        # --- Chart for speedup ---
        chart1 = workbook.add_chart({"type": "scatter", "subtype": "straight_with_markers"})
        chart1.add_series({
            "name": "Speedup",
            "categories": ["Data", 1 + row_offset, 0 + col_offset, len(df) + row_offset, 0 + col_offset],
            "values": ["Data", 1 + row_offset, 2 + col_offset, len(df) + row_offset, 2 + col_offset],
            "marker": {"type": "square", "size": 6},   # square markers
            "line": {"color": "blue"},                 # line color
        })

        chart1.set_x_axis({"name": "Threads"})
        chart1.set_y_axis({"name": "Speedup"})
        

        # --- Chart for time ---
        chart2 = workbook.add_chart({"type": "scatter", "subtype": "straight_with_markers"})
        chart2.add_series({
            "name": "Time (s)",
            "categories": ["Data", 1 + row_offset, 0 + col_offset, len(df) + row_offset, 0 + col_offset],  # x = threads
            "values":     ["Data", 1 + row_offset, 1 + col_offset, len(df) + row_offset, 1 + col_offset],  # y = total
            "marker": {"type": "diamond", "size": 6},  # diamond markers
            "line": {"color": "red"},                  # line color
        })

        chart2.set_x_axis({"name": "Threads"})
        chart2.set_y_axis({"name": "Time (s)"})

        # Insert charts into worksheet
        worksheet.insert_chart("B12", chart1)
        worksheet.insert_chart("K12", chart2)  # place second chart below the first

    return 

