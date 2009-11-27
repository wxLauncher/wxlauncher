import sqlite3, os, sys, tempfile, atexit, traceback, shutil
from optparse import OptionParser

import logging
NOTICE = 25

import HTMLParser

class Tag:
  def __init__(self, name, attrs, data=None):
    self.name = name
    self.attributes = attrs
    self.data = data
    
class OutputParser(HTMLParser.HTMLParser):
  """The class is designed to be used as a base class.  It will output the same html structure as the input file into a file like object (only needs write)."""
      
  def __init__(self, file, *args, **kwargs):
    HTMLParser.HTMLParser.__init__(self, *args, **kwargs)
    
    if hasattr(file, 'write'):
      self.outputfile = file
    else:
      raise Exception("file is not a file like object with a write function")
    
    self.tagstack = list()
  
  def handle_starttag(self, tag, attrs):
    logging.debug(" Handle starttag: %s", tag)
    logging.debug("  %s", attrs)
    self.tagstack.append(Tag(tag, attrs))
  def handle_startendtag(self, tag, attrs):
    logging.debug(" Handle start and end tag: %s", tag)
    logging.debug("  %s", attrs)
    self.write_tag(Tag(tag, attrs))
  def handle_endtag(self, tagname):
    """Finds the matching tag. If the tags are miss matched, function will go down the stack until it finds the match"""
    logging.debug(" End tag: %s", tagname)
    tag = self.find_match(tagname)
    self.write_tag(tag)
  def handle_data(self, data):
    logging.debug(" Data: %s", data)
    if len(self.tagstack) > 0:
      tag = self.tagstack.pop()
      if tag.data == None:
        tag.data = data
      else:
        tag.data += data
      self.tagstack.append(tag)
    else:
      self.outputfile.write(data)
  def find_match(self, tagtofind, indent=0):
    """Recursive function to match the tag"""
    tag = self.tagstack.pop()
    if not tag.name == tagtofind:
      logging.warning(" %smismatched tag found (expected %d, found %s)", " "*indent, tagtofind, tag.name)
      self.write_tag(tag)
      tag = self.find_match(tagtofind)
      
    return tag
  def write_tag(self, tag):
    """When tag stack is empty, writes tag to file passed in the constructor. When tag stack is not empty formats the tag and sets (or if the next tag.data is not None, appends) the formated string."""
    if tag.data == None:
      str = "<%s%s />" %(tag.name, self.format_attributes(tag))
    else:
      str = "<%s%s>%s</%s>" %(
        tag.name, self.format_attributes(tag), tag.data, tag.name)
        
    if len(self.tagstack) > 0:
      tag = self.tagstack.pop()
      if tag.data == None:
        tag.data = str
      else:
        tag.data += str
      self.tagstack.append(tag)
    else:
      self.outputfile.write(str)
  def format_attributes(self, tag):
    """Returns the attributes formatted to be placed in a tag."""
    ret = ""
    if len(tag.attributes) > 0:
      for name, value in tag.attributes:
        ret = "%s %s=\"%s\"" % (ret, name, value)
    return ret
try:
  import markdown
except ImportError:
  print "ERROR: Unable to import markdown the markup parser."
  print " Make sure that markdown has been installed"
  print "  see the ReadMe.txt for more information"
  raise

def main(argv):
  parser = OptionParser(usage="%prog <jobtype> <outfile> <indir> [options]")
  parser.add_option("-t", "--temp",
    help="use TEMPDIR to store the intermedite build files",
    metavar="TEMPDIR")
  parser.add_option("-q", "--quiet", action="store_false",
    dest="quiet", default=False,
    help="don't print status messages to stdout")

  (options, args) = parser.parse_args(argv)

  if  len(args) != 4 :
    parser.error("Incorrect number of arguments")

  options.type = args[1]
  options.outfile = args[2]
  options.indir = args[3]
  
  logging.basicConfig(level=logging.DEBUG,
    format='%(levelname)7s:%(message)s')
  logging.addLevelName(NOTICE, "NOTICE")

  if options.type == "build":
    pass
  elif options.type == "rebuild":
    pass
  elif options.type == "clean":
    pass
  else:
    parser.error("jobtype must be one of: build, rebuild, or clean")

  if not options.temp:
    logging.info("No working directory set. Creating one in system temp.")
    options.temp = tempfile.mkdtemp()

    def cleanup(dir):
      if os.path.exists(dir):
        os.rmdir(dir)

    atexit.register(cleanup, options.temp)

  logging.info("Doing a '%s'", options.type)
  logging.info("Using '%s' as working directory", options.temp )
  logging.info("Using '%s' as output file", options.outfile )
  logging.info("Using '%s' as input directory", options.indir )

  if options.type == "build":
    call_logging_exceptions(build, options)
  elif options.type == "rebuild":
    call_logging_exceptions(rebuild, options)
  else:
    call_logging_exceptions(clean, options)

def call_logging_exceptions(func, *args, **kwargs):
  """Protects the caller from all exceptions that occur in the callee. Logs the exceptions that do occur."""
  try:
    func(*args, **kwargs)
  except:
    ( type, value, tb ) = sys.exc_info()
    logging.error("***EXCEPTION:")
    logging.error(" %s: %s", type.__name__, value)
    for filename, line, function, text in traceback.extract_tb(tb):
      logging.error("%s:%d %s", os.path.basename(filename), line, function)
      logging.error("   %s", text)
    
def rebuild(options):
  logging.log(NOTICE, "Rebuilding")
  call_logging_exceptions(clean, options)
  call_logging_exceptions(build, options)

def clean(options):
  logging.log(NOTICE, "Cleaning..")
  logging.info("Removing outfile: %s", options.outfile )
  if os.path.exists(options.outfile):
    if os.path.isfile(options.outfile):
      os.remove(options.outfile)
    else:
      logging.error(" <outfile> is not a file. Make sure your parameters are in the correct order")
      sys.exit(2)
  else:
    logging.info(" Outfile (%s) does not exist", options.outfile)

  logging.info("Removing workdirectory: %s", options.temp )
  if os.path.exists(options.temp):
    if os.path.isdir(options.temp):
      shutil.rmtree(options.temp, ignore_errors=True)
    else:
      logging.error("tempdir is not a file. Make sure your parameters are in the correct order")
      sys.exit(2)
  else:
    logging.info(" Work directory (%s) does not exist", options.temp)


def build(options):
  """Compiles the files in options.indir to the archive output options.outfile.

  Compiled in several stages:
    stage1: Transform all input files with markdown placing the results into options.temp+"/stage1".
    stage2: Parses and strips the output of stage1 to build the c-array that contains the compiled names for the detailed help.
    stage3: Parses the output of stage1 to grab the images that are refered to in the the output of stage1
    stage4: Parses the output of stage1 to fix the relative hyperlinks in the output so that they will refer correctly to the correct files when in output file.
    stage5: Generate the index and table of contents for the output file.
    stage6: Zip up the output of stage5 and put it in the output location.
    """
  logging.log(NOTICE, "Building...")
  files = generate_paths(options)

  if should_build(options, files):
    input_files = generate_input_files_list(options)

    helparray = list()
    logging.info(" Processing input files:")
    for file in input_files:
      logging.info("  %s", file)
      logging.info("   Stage 1")
      
      name1 = process_input_stage1(file, options, files)
      logging.info("   Stage 2")
      
      name2 = process_input_stage2(name1, options, files, helparray)
      logging.info("   Stage 3")
      
      name3 = process_input_stage3(name2, options, files)
      logging.info("   Stage 4")

    print helparray


def generate_paths(options):
  """Generates the names of the paths that will be needed by the compiler during it's run, storing them in a dictionary that is returned."""
  paths = dict()

  for i in range(1, 7):
    paths['stage%d'%(i)] = os.path.join(options.temp, 'stage%d'%(i))

  for path in paths.values():
    if not os.path.exists(path):
      logging.debug(" Making %s", path)
      os.makedirs(path)

  return paths

def should_build(options, files):
  """Checks the all of the files touched by the compiler to see if we need to rebuild. Returns True if so."""
  return True

def generate_input_files_list(options):
  """Returns a list of all input (.help) files that need to be parsed by markdown."""
  file_list = list()

  for path, dirs, files in os.walk(options.indir):
    for file in files:
      if os.path.splitext(file)[1] == ".help":
        # I only want the .help files
        file_list.append(os.path.join(path, file))
        
        
  return file_list
  
def change_filename(filename, newext, orginaldir, destdir):
  """Returns the filename after transforming it to be in destdir and making sure the folders required all exist."""
  logging.debug("   change_filename('%s', '%s', '%s', '%s')", filename, newext, orginaldir, destdir)
  outfile_name1 = filename.replace(orginaldir, ".") # files relative name
  logging.debug(outfile_name1)
  outfile_name2 = os.path.splitext(outfile_name1)[0] #file's name without ext
  logging.debug(outfile_name2)
  outfile_name3 = outfile_name2 + newext
  logging.debug(outfile_name3)
  outfile_name4 = os.path.join(destdir, outfile_name3)
  logging.debug(outfile_name4)
  outfile_name = os.path.normpath(outfile_name4)
  logging.debug(outfile_name)
  
  # make sure that the folder exists to output 
  outfile_path = os.path.dirname(outfile_name)
  if os.path.exists(outfile_path):
    if os.path.isdir(outfile_path):
      pass # do nothing because the folder already exists
    else:
      raise Exception("%s already exists but is not a directory"%(outfile_path))
  else:
    os.makedirs(outfile_path)
    
  return outfile_name

def process_input_stage1(file, options, files):
  infile = open(file, mode="r")
  input = infile.read()
  infile.close()

  output = markdown.markdown(input)

  outfile_name = change_filename(file, ".stage1", options.indir, files['stage1'])
  
  outfile = open(outfile_name, mode="w")
  outfile.write(output)
  outfile.close()
  return outfile_name

def process_input_stage2(file, options, files, helparray):
  infile = open(file, mode="r")
  input = infile.read()
  infile.close()
  
  outname = change_filename(file, ".stage2", files['stage1'], files['stage2'])
  outfile = open(outname, mode="w")

  class Stage2Parser(OutputParser):
    def __init__(self, *args, **kwargs):
      OutputParser.__init__(self, *args, **kwargs)
      self.control = None

    def handle_starttag(self, tag, attrs):
      if tag == "meta":
        if attrs[0][0] == "name" and attrs[0][1] == "control" and attrs[1][0] == "content":
          self.control = attrs[1][1]
      else:
        OutputParser.handle_starttag(self, tag, attrs)

  parser = Stage2Parser(file=outfile)
  parser.feed(input)
  parser.close()
  outfile.close()

  if parser.control:
    helparray.append((parser.control, file))
    logging.debug(" Control name %s", parser.control)
      
  return outname

def process_input_stage3(file, options, files):
  infile = open(file, mode="r")
  input = infile.read()
  infile.close()

  class Stage3Parser(HTMLParser.HTMLParser):
    def __init__(self, options, files):
      HTMLParser.HTMLParser.__init__(self)
      self.options = options
      self.files = files

    def handle_startendtag(self, tag, attrs):
      """Find the image and copy it to the stage3 folder where it should
      be in the file output."""
      if tag == "img":
        location1 = os.path.join(self.options.indir, attrs[1][1])
        location = os.path.normpath(location1)

        # check to make sure that the image I am including was in the onlinehelp folder,
        # if not change the dst name so that it will be located correctly in the stage3 directory
        if location.startswith(self.options.indir):
          dst1 = os.path.join(self.files['stage3'], attrs[1][1])
        else:
          # get extention
          basename = os.path.basename(attrs[1][1])
          (name, ext) = os.path.split(basename)
          (file, outname) = tempfile.mkstemp(ext, name, self.files['stage3'])
          dst1 = outname.replace(os.getcwd(), ".") # make into a relative path

        dst = os.path.normpath(dst1)
        logging.debug(" Image (%s) should be in %s and copying to %s", attrs[0][1], location, dst)
        try:
          shutil.copy2(location, dst)
        except:
          traceback.print_exc()
          logging.error(" '%s' does not exist", location )
          sys.exit(3)

  parser = Stage3Parser(options, files)
  parser.feed(input)
  parser.close()

if __name__ == "__main__":
  main(sys.argv)


