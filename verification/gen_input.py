# Ryan Bentz, Jamie Williams, Ashwini Nayak Kota, Jeff Gragg
# ECE 485/858
# Final Project
# 2018-06-13

# For ECE 585 Final Project
#   This function generates a sample input file for the DDR4 Memory controller
import random

def gen_input():
    """ Function to generate sample input to a DDR4 Controller """
    
    print("Running input generation program ...")
    print("\n WARNING: This program can overwrite files with same name!")

    filename = ""
    if not filename:
        filename = input("Enter the name of the file to save to: ")
    gen_file = open(filename, "w")
    num_lines = input("Number of lines of output: ")

    """ Define parameters """
    clock = 1               # Starting clock value
    #max_incr = 50          # Maximum clock increment (usual)
    max_incr = 100000         # Max clock incrment (to promote clock rollover
    request_list = ("IFETCH", "READ", "WRITE") # All possible request types
    addr_gen = random.randrange(0x0000000,0x1FFFFFFFF)
    
    """ Generate lines for sample input """
    for line in range(1, int(num_lines)+1):
        # Increment clock
        clock = clock + random.randint(1,max_incr)
        # Determine which type of request is being issued
        which_req = random.randint(0,2)
        request = request_list[which_req]
        # Generate address

        # Full random address generator
        #addr_gen = random.randrange(0x0000000,0x1FFFFFFFF)

        # **Local generated addresses
        addr_gen = close_addr(addr_gen)
        
        address = int(addr_gen)
        line_str = str(clock) + " " + request + " 0x" + format(address, '09X')
        #print(line_str)
        gen_file.write(line_str + "\n")
    gen_file.close()
    
def close_addr(addr_gen):
    max_move = 0x1FFFFFF
    rand_sign = (-1, 1)
    addr_gen += random.choice(rand_sign) * random.randrange(0x0,max_move)
    if addr_gen > 0x1FFFFFFFF or addr_gen < 0x0:
            addr_gen = 0x1FFFFFFFF/2
    return addr_gen
        
def test_addresses():
    rand_sign = (-1, 1)
    addr_gen = random.randrange(0x0000000,0x1FFFFFFFF)
    print("Start value: " + format(addr_gen, '09X'))
    for x in range(1,20):

        addr_gen += random.choice(rand_sign) * random.randrange(0x0,0x7FFFFFF)
        if addr_gen > 0x1FFFFFFFF or addr_gen < 0x0:
            addr_gen = 0x1FFFFFFFF/2
        addr_gen = int(addr_gen)
        print("Value: " + format(addr_gen, '09X'))

gen_input()
#test_addresses()
