/** @page project_r Template project for RIDE toolchain with Raisonance compiler

  @par Project description

  This folder contains a standard RIDE template workspace with a project 
  including all the user-modifiable files that are necessary to create a new project.
  This project template can be used by mean of minor updates in the library files
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
  
  - project\RIDE
     - project.rprj              Workspace file
     - project.rapp              Project file  


  @par How to use it ?

  - Open the RIDE workspace (project.rprj)
  - Rebuild all files: : Project -> Build (Alt+F9)
  - Load project image: Debug->Load(Ctrl+L)
  - Run program: Debug->Start (Ctrl+D)

  */

/******************* (C) COPYRIGHT 2009 STMicroelectronics *****END OF FILE****/