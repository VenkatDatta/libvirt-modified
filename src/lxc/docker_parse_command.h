#ifndef __DOCKER_PARSE_COMMAND_H__
# define __DOCKER_PARSE_COMMAND_H__
# include "domain_conf.h"
virDomainDefPtr
dockerParseCommandLine(virCapsPtr caps,
                     virDomainXMLOptionPtr xmlopt,
                     const char *config);
                    
virDomainDefPtr dockerParseCommandLineString(virCapsPtr caps,
                                           virDomainXMLOptionPtr xmlopt,
                                           const char *config);

int dockerParseMem(virDomainDefPtr dom,
                   const char *val);

int dockerParseVCpus(virDomainDefPtr dom ,const char *val,virDomainXMLOptionPtr xmlopt);

#endif
                                        