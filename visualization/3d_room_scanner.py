import open3d as o3d
import numpy as np
import time
import serial
import math

# Configurable variables
xMEASUREMENTS = 3
xDISTANCE = 300

# Create initial values of prev's, just in case the first one is a poor value
prevY = 0
prevZ = 0

# Create and open s——ENSURE CORRECT PORT AND BAUD RATE FOR YOUR SYSTEM
s = serial.Serial('COM6', 115200, timeout = 10)
                            
print("Opening: " + s.name)
print("Please begin input!")

# Create the file in write-mode
f = open("tof_radar.xyz", "w")

# reset the buffers of the UART port to delete the remaining data in the buffers
s.reset_output_buffer()
s.reset_input_buffer()

# Do this xMEASUREMENTS times of 32
for i in range(xMEASUREMENTS):
# recieve characters from UART of MCU
    for j in range(32):
        # Read and decode each measurement
        value = s.readline().decode().strip()
        
        # If the value is garbage
        if(value == 'g'):
            x = xDISTANCE*i
            y = prevY
            z = prevZ
            
        # If it is not a garbage value
        else:
            # Transer the data properly
            data = float(value)
            
            # Create each point  
            x = xDISTANCE*i
            y = data * math.cos(j*math.pi/16)
            z = data * math.sin(j*math.pi/16)
            
            # Save previous values that can be used if there are "bad" measurements
            prevY = y
            prevZ = z
        
        # Now write them to the file
        f.write(f"{x} {y} {z}\n")
       
    # Add a delay for the motor spinning back
    time.sleep(5)   

# Close the file
f.close()

#close the port
print("Closing: " + s.name)
s.close()

rpd = o3d.io.read_point_cloud("tof_radar.xyz", format="xyz")

#Define coordinates to connect lines       
lines = []  

# Go as many times as there are x-measurements
for x in range(xMEASUREMENTS):
    # Connect the rings
    for i in range(32): 
        # Connect the rings of the xMeasureth 
        lines.append([i + 32 * x, (i + 1) % 32 + 32 * x])  
    
# Go as many times ( - 1) as there are x-measurements
for x in range(xMEASUREMENTS - 1):
    # Now connect the "vertical" lines 
    for i in range(32):
        # Connect the "vertical"
        lines.append([i + 32 * x, i + 32 * (x + 1)])

#This line maps the lines to the 3d coordinate vertices
line_set = o3d.geometry.LineSet(points=o3d.utility.Vector3dVector(np.asarray(rpd.points)),lines=o3d.utility.Vector2iVector(lines))

#Lets see what our point cloud data with lines looks like graphically (add axis if necessary)
o3d.visualization.draw_geometries([line_set])