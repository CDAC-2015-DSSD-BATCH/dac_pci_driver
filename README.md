# dac_pci_driver
Driver for PCI Based Data Aquisition Card.

------------------------------- Requirements --------------------------------------

PCI Card name: ESA-PCI-DAS, from Electro System Associates pvt ltd. Bangalore, India.

System Used: Intel Dual Core, 4GB RAM.

Operating System: OpenSuse.

Kernel: Linux 3.3.6

---------------------------- Note About The Repository ----------------------------

Notes/ -> Provides basic Notes / References, Sample Codes which were used to build 
	  the project.

	RTAI-ESA-DAS/ -> was the Code written for RTAI systems.
			 Since there was no Proper Documentation/User Manual/Data Sheet 
			 of the DAS PCI Card, we had to go through the RTAI code to check
			 which BAR Address were they using, assuming that RTAI Code was 
			 working perfectly, we used the Samw BAR Address.

	Sample_Codes_For_PCI/ -> Sample codes to program and understand PCI framework.
			Probing, Removal techniques are discussed in the code.

	PC_Bus_Demystified_Doug_Abbott.pdf -> Reference Book to make use of PC BUS.

	User Manual For ESAPCIDAS.PDF -> Data Sheet of ESA-DAS-PCI.

SnapShots/ -> Pictures of few moments while doing the Project.

Source/ -> Contains the Source Code for ESA-PCI-DAS Driver.
	
	doc/ -> Contains Document about ESA-PCI-DAS Driver.	

	ESA_PCI_DAS.c -> Source Code

	Makefile -> Command to Compile the code, using 'make modules' and 'make clean' commands
