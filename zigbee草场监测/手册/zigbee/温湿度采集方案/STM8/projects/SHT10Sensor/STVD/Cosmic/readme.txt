/** @page project Template project for ST Visual Develop (STVD) toolchain with Cosmic compiler

  @par Project description

  This folder contains a standard STVD template workspace with a project 
  including all the user-modifiable files that are necessary to create a new project.
  This project templates can be used by mean of minor updates in the library files
  to run the StdPeriph_Lib examples, or custom user applications. 
  
  @b Tip: You can tailor the provided project template to run this example, for
          more details please refer to "stm8l10x_stdperiph_lib_um.chm" user manual;
          select "Peripheral Examples" then follow the instructions provided in
          "How to proceed" section.
  
  @par Directories contents

  - Template
     - main.c                   Main file containing the "main" function
     - stm8l10x_conf.h          Library Configuration file
     - stm8l10x_it.c            Interrupt routines source
     - stm8l10x_it.h            Interrupt routines declaration
     
  - Template\STVD\Cosmic
     - project.stw              Workspace file
     - project.stp              Project file 
     - stm8_interrupt_vector.c  Interrupt vectors table


  @par How to use it ?

  - Open the STVD workspace
  - Select your debug instrument: Debug instrument-> Target Settings, select the 
    target you want to use for debug session (Swim Stice or Swim Rlink)
  - Rebuild all files: Build-> Rebuild all. 
  - Load project image: Debug->Start Debugging
  - Run program: Debug->Run (Ctrl+F5)
  
  @b Tip: If it is your first time using STVD, you have to confirm the default 
  toolset and path information that will be used when building your application, 
  to do so:
    - Select Tools-> Options
    - In the Options window click on the Toolset tab
    - Select your toolset from the Toolset list box.
    If the path is incorrect you can type the correct path in the Root Path 
    field, or use the browse button to locate it.
    - In the subpath fields, type the correct subpath if necessary 
  
 
  */

/******************* (C) COPYRIGHT 2009 STMicroelectronics *****END OF FILE****/
