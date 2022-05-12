import sys
import json
import os

if len(sys.argv) < 2:
  print("Usage: parse_tracing.py <log dir> <app name> <input file>")
  sys.exit(1)

json_content = {
  "traceEvents": [],
  "displayTimeUnit": "ms"
}

def parse_log(log_file):
  f = open(log_file, 'r')
  lines = f.readlines()
  for line in lines:
    if "[Tracing]" in line:
      cleaned_line = line.split('[Tracing]')[1]
      cleaned_line = cleaned_line.strip().replace("'", '"')
      try :
        json_content["traceEvents"].append(json.loads(cleaned_line))
      except Exception as e:
        print(cleaned_line)
        print(e)
        sys.exit(1)
  
  f.close()


if __name__ == "__main__":
  log_dir = sys.argv[1]
  app_name = sys.argv[2]
  input_file = sys.argv[3]
  if not os.path.isdir(log_dir):
    print("{} is not a directory".format(log_dir))
    sys.exit(1)

  file_prefix = "{}_{}".format(app_name, input_file)
  for log_file in os.listdir(log_dir):
    if file_prefix in log_file and log_file.endswith(".log"):
      print("Parsing " + log_file)
      parse_log(os.path.join(log_dir, log_file))

  print("Parsed {} events".format(len(json_content["traceEvents"])))
  outfile = os.path.join(log_dir, file_prefix + ".json")
  with open(outfile, 'w') as f:
    json.dump(json_content, f)
  print("Tracing data written to: " + outfile)