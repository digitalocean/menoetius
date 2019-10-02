#include "storage.h"

#include <assert.h>

//
// This code is based on sglib.h; it was produced by running
// gcc with -E; which dumps post-macro code;
//
// The code was then extended to support arrays of indicies
//
// It really needs some testing, and then some code clean up
//
/*

  This is SGLIB version 1.0.4

  (C) by Marian Vittek, Bratislava, http://www.xref-tech.com/sglib, 2003-5

  License Conditions: You can use a verbatim copy (including this
  copyright notice) of sglib freely in any project, commercial or not.
  You can also use derivative forms freely under terms of Open Source
  Software license or under terms of GNU Public License.  If you need
  to use a derivative form in a commercial project, or you need sglib
  under any other license conditions, contact the author.



*/

static void sglib___StorageItem_fix_left_insertion_discrepancy( StorageItem** tree, int index )
{
	{
		StorageItem *t, *tl, *a, *b, *c, *ar, *bl, *br, *cl, *cr;
		(void)( bl );
		(void)( ar );
		t = *tree;
		tl = t->left[index];
		if( t->right[index] != NULL && ( t->right[index]->color_field[index] ) == 1 ) {
			if( ( tl->color_field[index] ) == 1 ) {
				if( ( tl->left[index] != NULL && ( tl->left[index]->color_field[index] ) == 1 ) ||
					( tl->right[index] != NULL &&
					  ( tl->right[index]->color_field[index] ) == 1 ) ) {
					{
						( t->left[index]->color_field[index] ) = ( 0 );
					};
					{
						( t->right[index]->color_field[index] ) = ( 0 );
					};
					{
						( t->color_field[index] ) = ( 1 );
					};
				}
			}
		}
		else {
			if( ( tl->color_field[index] ) == 1 ) {
				if( tl->left[index] != NULL && ( tl->left[index]->color_field[index] ) == 1 ) {
					a = t;
					b = tl;
					c = tl->left[index];
					br = b->right[index];
					a->left[index] = br;
					b->left[index] = c;
					b->right[index] = a;
					{
						( a->color_field[index] ) = ( 1 );
					};
					{
						( b->color_field[index] ) = ( 0 );
					};
					*tree = b;
				}
				else if( tl->right[index] != NULL &&
						 ( tl->right[index]->color_field[index] ) == 1 ) {
					a = t;
					b = tl;
					ar = a->right[index];
					bl = b->left[index];
					c = b->right[index];
					cl = c->left[index];
					cr = c->right[index];
					b->right[index] = cl;
					a->left[index] = cr;
					c->left[index] = b;
					c->right[index] = a;
					{
						( c->color_field[index] ) = ( 0 );
					};
					{
						( a->color_field[index] ) = ( 1 );
					};
					*tree = c;
				}
			}
		}
	};
}
static void sglib___StorageItem_fix_right_insertion_discrepancy( StorageItem** tree, int index )
{
	{
		StorageItem *t, *tl, *a, *b, *c, *ar, *bl, *br, *cl, *cr;
		(void)( bl );
		(void)( ar );
		t = *tree;
		tl = t->right[index];
		if( t->left[index] != NULL && ( t->left[index]->color_field[index] ) == 1 ) {
			if( ( tl->color_field[index] ) == 1 ) {
				if( ( tl->right[index] != NULL && ( tl->right[index]->color_field[index] ) == 1 ) ||
					( tl->left[index] != NULL && ( tl->left[index]->color_field[index] ) == 1 ) ) {
					{
						( t->right[index]->color_field[index] ) = ( 0 );
					};
					{
						( t->left[index]->color_field[index] ) = ( 0 );
					};
					{
						( t->color_field[index] ) = ( 1 );
					};
				}
			}
		}
		else {
			if( ( tl->color_field[index] ) == 1 ) {
				if( tl->right[index] != NULL && ( tl->right[index]->color_field[index] ) == 1 ) {
					a = t;
					b = tl;
					c = tl->right[index];
					br = b->left[index];
					a->right[index] = br;
					b->right[index] = c;
					b->left[index] = a;
					{
						( a->color_field[index] ) = ( 1 );
					};
					{
						( b->color_field[index] ) = ( 0 );
					};
					*tree = b;
				}
				else if( tl->left[index] != NULL && ( tl->left[index]->color_field[index] ) == 1 ) {
					a = t;
					b = tl;
					ar = a->left[index];
					bl = b->right[index];
					c = b->left[index];
					cl = c->right[index];
					cr = c->left[index];
					b->left[index] = cl;
					a->right[index] = cr;
					c->right[index] = b;
					c->left[index] = a;
					{
						( c->color_field[index] ) = ( 0 );
					};
					{
						( a->color_field[index] ) = ( 1 );
					};
					*tree = c;
				}
			}
		}
	};
}
static int sglib___StorageItem_fix_left_deletion_discrepancy( StorageItem** tree, int index )
{
	int res;
	{
		StorageItem *t, *a, *b, *c, *d, *ar, *bl, *br, *cl, *cr, *dl, *dr;
		(void)( ar );
		t = a = *tree;
		( (void)sizeof( ( t != NULL ) ? 1 : 0 ), __extension__( {
			  if( t == NULL )
				  __assert_fail(
					  "t!=NULL", "src/server/storage.c", 16, __extension__ __PRETTY_FUNCTION__ );
		  } ) );
		ar = a->left[index];
		b = t->right[index];
		if( b == NULL ) {
			( (void)sizeof( ( ( t->color_field[index] ) == 1 ) ? 1 : 0 ), __extension__( {
				  if( ( t->color_field[index] ) == 1 )
					  ;
				  else
					  __assert_fail( "SGLIB___GET_VALUE(t->color_field)==1",
									 "src/server/storage.c",
									 16,
									 __extension__ __PRETTY_FUNCTION__ );
			  } ) );
			{
				( t->color_field[index] ) = ( 0 );
			};
			res = 0;
		}
		else {
			bl = b->right[index];
			br = b->left[index];
			if( ( b->color_field[index] ) == 1 ) {
				if( br == NULL ) {
					*tree = b;
					{
						( b->color_field[index] ) = ( 0 );
					};
					b->left[index] = a;
					a->right[index] = br;
					res = 0;
				}
				else {
					c = br;
					( (void)sizeof( ( c != NULL && ( c->color_field[index] ) == 0 ) ? 1 : 0 ),
					  __extension__( {
						  if( c != NULL && ( c->color_field[index] ) == 0 )
							  ;
						  else
							  __assert_fail( "c!=NULL && SGLIB___GET_VALUE(c->color_field)==0",
											 "src/server/storage.c",
											 16,
											 __extension__ __PRETTY_FUNCTION__ );
					  } ) );
					cl = c->right[index];
					cr = c->left[index];
					if( ( cl == NULL || ( cl->color_field[index] ) == 0 ) &&
						( cr == NULL || ( cr->color_field[index] ) == 0 ) ) {
						*tree = b;
						b->left[index] = a;
						{
							( b->color_field[index] ) = ( 0 );
						};
						a->right[index] = c;
						{
							( c->color_field[index] ) = ( 1 );
						};
						res = 0;
					}
					else if( cl != NULL && ( cl->color_field[index] ) == 1 ) {
						if( cr != NULL && ( cr->color_field[index] ) == 1 ) {
							d = cr;
							dl = d->right[index];
							dr = d->left[index];
							*tree = d;
							{
								( d->color_field[index] ) = ( 0 );
							};
							d->right[index] = b;
							c->left[index] = dl;
							d->left[index] = a;
							a->right[index] = dr;
							res = 0;
						}
						else {
							*tree = c;
							c->right[index] = b;
							c->left[index] = a;
							b->right[index] = bl;
							b->left[index] = cl;
							a->right[index] = cr;
							{
								( cl->color_field[index] ) = ( 0 );
							};
							res = 0;
						}
					}
					else if( cr != NULL && ( cr->color_field[index] ) == 1 ) {
						( (void)sizeof( ( cl == NULL || ( cl->color_field[index] ) == 0 ) ? 1 : 0 ),
						  __extension__( {
							  if( cl == NULL || ( cl->color_field[index] ) == 0 )
								  ;
							  else
								  __assert_fail(
									  "cl==NULL || SGLIB___GET_VALUE(cl->color_field)==0",
									  "src/server/storage.c",
									  16,
									  __extension__ __PRETTY_FUNCTION__ );
						  } ) );
						d = cr;
						dl = d->right[index];
						dr = d->left[index];
						*tree = d;
						{
							( d->color_field[index] ) = ( 0 );
						};
						d->right[index] = b;
						c->left[index] = dl;
						d->left[index] = a;
						a->right[index] = dr;
						res = 0;
					}
					else {
						( (void)sizeof( ( 0 ) ? 1 : 0 ), __extension__( {
							  if( 0 )
								  ;
							  else
								  __assert_fail( "0",
												 "src/server/storage.c",
												 16,
												 __extension__ __PRETTY_FUNCTION__ );
						  } ) );
						res = 0;
					}
				}
			}
			else {
				if( ( bl == NULL || ( bl->color_field[index] ) == 0 ) &&
					( br == NULL || ( br->color_field[index] ) == 0 ) ) {
					res = ( ( a->color_field[index] ) == 0 );
					{
						( a->color_field[index] ) = ( 0 );
					};
					{
						( b->color_field[index] ) = ( 1 );
					};
				}
				else if( bl != NULL && ( bl->color_field[index] ) == 1 ) {
					if( br == NULL || ( br->color_field[index] ) == 0 ) {
						*tree = b;
						{
							( b->color_field[index] ) = ( ( a->color_field[index] ) );
						};
						{
							( a->color_field[index] ) = ( 0 );
						};
						b->left[index] = a;
						a->right[index] = br;
						{
							( bl->color_field[index] ) = ( 0 );
						};
						res = 0;
					}
					else {
						( (void)sizeof( ( bl != NULL ) ? 1 : 0 ), __extension__( {
							  if( bl != NULL )
								  ;
							  else
								  __assert_fail( "bl!=NULL",
												 "src/server/storage.c",
												 16,
												 __extension__ __PRETTY_FUNCTION__ );
						  } ) );
						( (void)sizeof( ( br != NULL ) ? 1 : 0 ), __extension__( {
							  if( br != NULL )
								  ;
							  else
								  __assert_fail( "br!=NULL",
												 "src/server/storage.c",
												 16,
												 __extension__ __PRETTY_FUNCTION__ );
						  } ) );
						( (void)sizeof( ( ( bl->color_field[index] ) == 1 ) ? 1 : 0 ),
						  __extension__( {
							  if( ( bl->color_field[index] ) == 1 )
								  ;
							  else
								  __assert_fail( "SGLIB___GET_VALUE(bl->color_field)==1",
												 "src/server/storage.c",
												 16,
												 __extension__ __PRETTY_FUNCTION__ );
						  } ) );
						( (void)sizeof( ( ( br->color_field[index] ) == 1 ) ? 1 : 0 ),
						  __extension__( {
							  if( ( br->color_field[index] ) == 1 )
								  ;
							  else
								  __assert_fail( "SGLIB___GET_VALUE(br->color_field)==1",
												 "src/server/storage.c",
												 16,
												 __extension__ __PRETTY_FUNCTION__ );
						  } ) );
						c = br;
						cl = c->right[index];
						cr = c->left[index];
						*tree = c;
						{
							( c->color_field[index] ) = ( ( a->color_field[index] ) );
						};
						{
							( a->color_field[index] ) = ( 0 );
						};
						c->right[index] = b;
						c->left[index] = a;
						b->left[index] = cl;
						a->right[index] = cr;
						res = 0;
					}
				}
				else {
					( (void)sizeof( ( br != NULL && ( br->color_field[index] ) == 1 ) ? 1 : 0 ),
					  __extension__( {
						  if( br != NULL && ( br->color_field[index] ) == 1 )
							  ;
						  else
							  __assert_fail( "br!=NULL && SGLIB___GET_VALUE(br->color_field)==1",
											 "src/server/storage.c",
											 16,
											 __extension__ __PRETTY_FUNCTION__ );
					  } ) );
					c = br;
					cl = c->right[index];
					cr = c->left[index];
					*tree = c;
					{
						( c->color_field[index] ) = ( ( a->color_field[index] ) );
					};
					{
						( a->color_field[index] ) = ( 0 );
					};
					c->right[index] = b;
					c->left[index] = a;
					b->left[index] = cl;
					a->right[index] = cr;
					res = 0;
				}
			}
		}
	};
	return ( res );
}

static int sglib___StorageItem_fix_right_deletion_discrepancy( StorageItem** tree, int index )
{
	int res;
	{
		StorageItem *t, *a, *b, *c, *d, *ar, *bl, *br, *cl, *cr, *dl, *dr;
		(void)( ar );
		t = a = *tree;
		( (void)sizeof( ( t != NULL ) ? 1 : 0 ), __extension__( {
			  if( t != NULL )
				  ;
			  else
				  __assert_fail(
					  "t!=NULL", "src/server/storage.c", 16, __extension__ __PRETTY_FUNCTION__ );
		  } ) );
		ar = a->right[index];
		b = t->left[index];
		if( b == NULL ) {
			( (void)sizeof( ( ( t->color_field[index] ) == 1 ) ? 1 : 0 ), __extension__( {
				  if( ( t->color_field[index] ) == 1 )
					  ;
				  else
					  __assert_fail( "SGLIB___GET_VALUE(t->color_field)==1",
									 "src/server/storage.c",
									 16,
									 __extension__ __PRETTY_FUNCTION__ );
			  } ) );
			{
				( t->color_field[index] ) = ( 0 );
			};
			res = 0;
		}
		else {
			bl = b->left[index];
			br = b->right[index];
			if( ( b->color_field[index] ) == 1 ) {
				if( br == NULL ) {
					*tree = b;
					{
						( b->color_field[index] ) = ( 0 );
					};
					b->right[index] = a;
					a->left[index] = br;
					res = 0;
				}
				else {
					c = br;
					( (void)sizeof( ( c != NULL && ( c->color_field[index] ) == 0 ) ? 1 : 0 ),
					  __extension__( {
						  if( c != NULL && ( c->color_field[index] ) == 0 )
							  ;
						  else
							  __assert_fail( "c!=NULL && SGLIB___GET_VALUE(c->color_field)==0",
											 "src/server/storage.c",
											 16,
											 __extension__ __PRETTY_FUNCTION__ );
					  } ) );
					cl = c->left[index];
					cr = c->right[index];
					if( ( cl == NULL || ( cl->color_field[index] ) == 0 ) &&
						( cr == NULL || ( cr->color_field[index] ) == 0 ) ) {
						*tree = b;
						b->right[index] = a;
						{
							( b->color_field[index] ) = ( 0 );
						};
						a->left[index] = c;
						{
							( c->color_field[index] ) = ( 1 );
						};
						res = 0;
					}
					else if( cl != NULL && ( cl->color_field[index] ) == 1 ) {
						if( cr != NULL && ( cr->color_field[index] ) == 1 ) {
							d = cr;
							dl = d->left[index];
							dr = d->right[index];
							*tree = d;
							{
								( d->color_field[index] ) = ( 0 );
							};
							d->left[index] = b;
							c->right[index] = dl;
							d->right[index] = a;
							a->left[index] = dr;
							res = 0;
						}
						else {
							*tree = c;
							c->left[index] = b;
							c->right[index] = a;
							b->left[index] = bl;
							b->right[index] = cl;
							a->left[index] = cr;
							{
								( cl->color_field[index] ) = ( 0 );
							};
							res = 0;
						}
					}
					else if( cr != NULL && ( cr->color_field[index] ) == 1 ) {
						( (void)sizeof( ( cl == NULL || ( cl->color_field[index] ) == 0 ) ? 1 : 0 ),
						  __extension__( {
							  if( cl == NULL || ( cl->color_field[index] ) == 0 )
								  ;
							  else
								  __assert_fail(
									  "cl==NULL || SGLIB___GET_VALUE(cl->color_field)==0",
									  "src/server/storage.c",
									  16,
									  __extension__ __PRETTY_FUNCTION__ );
						  } ) );
						d = cr;
						dl = d->left[index];
						dr = d->right[index];
						*tree = d;
						{
							( d->color_field[index] ) = ( 0 );
						};
						d->left[index] = b;
						c->right[index] = dl;
						d->right[index] = a;
						a->left[index] = dr;
						res = 0;
					}
					else {
						( (void)sizeof( ( 0 ) ? 1 : 0 ), __extension__( {
							  if( 0 )
								  ;
							  else
								  __assert_fail( "0",
												 "src/server/storage.c",
												 16,
												 __extension__ __PRETTY_FUNCTION__ );
						  } ) );
						res = 0;
					}
				}
			}
			else {
				if( ( bl == NULL || ( bl->color_field[index] ) == 0 ) &&
					( br == NULL || ( br->color_field[index] ) == 0 ) ) {
					res = ( ( a->color_field[index] ) == 0 );
					{
						( a->color_field[index] ) = ( 0 );
					};
					{
						( b->color_field[index] ) = ( 1 );
					};
				}
				else if( bl != NULL && ( bl->color_field[index] ) == 1 ) {
					if( br == NULL || ( br->color_field[index] ) == 0 ) {
						*tree = b;
						{
							( b->color_field[index] ) = ( ( a->color_field[index] ) );
						};
						{
							( a->color_field[index] ) = ( 0 );
						};
						b->right[index] = a;
						a->left[index] = br;
						{
							( bl->color_field[index] ) = ( 0 );
						};
						res = 0;
					}
					else {
						( (void)sizeof( ( bl != NULL ) ? 1 : 0 ), __extension__( {
							  if( bl != NULL )
								  ;
							  else
								  __assert_fail( "bl!=NULL",
												 "src/server/storage.c",
												 16,
												 __extension__ __PRETTY_FUNCTION__ );
						  } ) );
						( (void)sizeof( ( br != NULL ) ? 1 : 0 ), __extension__( {
							  if( br != NULL )
								  ;
							  else
								  __assert_fail( "br!=NULL",
												 "src/server/storage.c",
												 16,
												 __extension__ __PRETTY_FUNCTION__ );
						  } ) );
						( (void)sizeof( ( ( bl->color_field[index] ) == 1 ) ? 1 : 0 ),
						  __extension__( {
							  if( ( bl->color_field[index] ) == 1 )
								  ;
							  else
								  __assert_fail( "SGLIB___GET_VALUE(bl->color_field)==1",
												 "src/server/storage.c",
												 16,
												 __extension__ __PRETTY_FUNCTION__ );
						  } ) );
						( (void)sizeof( ( ( br->color_field[index] ) == 1 ) ? 1 : 0 ),
						  __extension__( {
							  if( ( br->color_field[index] ) == 1 )
								  ;
							  else
								  __assert_fail( "SGLIB___GET_VALUE(br->color_field)==1",
												 "src/server/storage.c",
												 16,
												 __extension__ __PRETTY_FUNCTION__ );
						  } ) );
						c = br;
						cl = c->left[index];
						cr = c->right[index];
						*tree = c;
						{
							( c->color_field[index] ) = ( ( a->color_field[index] ) );
						};
						{
							( a->color_field[index] ) = ( 0 );
						};
						c->left[index] = b;
						c->right[index] = a;
						b->right[index] = cl;
						a->left[index] = cr;
						res = 0;
					}
				}
				else {
					( (void)sizeof( ( br != NULL && ( br->color_field[index] ) == 1 ) ? 1 : 0 ),
					  __extension__( {
						  if( br != NULL && ( br->color_field[index] ) == 1 )
							  ;
						  else
							  __assert_fail( "br!=NULL && SGLIB___GET_VALUE(br->color_field)==1",
											 "src/server/storage.c",
											 16,
											 __extension__ __PRETTY_FUNCTION__ );
					  } ) );
					c = br;
					cl = c->left[index];
					cr = c->right[index];
					*tree = c;
					{
						( c->color_field[index] ) = ( ( a->color_field[index] ) );
					};
					{
						( a->color_field[index] ) = ( 0 );
					};
					c->left[index] = b;
					c->right[index] = a;
					b->right[index] = cl;
					a->left[index] = cr;
					res = 0;
				}
			}
		}
	};
	return ( res );
}

static void sglib___StorageItem_add_recursive( StorageItem** tree, StorageItem* elem, int index )
{
	int cmp;
	StorageItem* t;
	t = *tree;
	if( t == NULL ) {
		{
			( elem->color_field[index] ) = ( 1 );
		};
		*tree = elem;
	}
	else {
		cmp = storage_item_cmp( elem, t, index );
		if( cmp < 0 || ( cmp == 0 && elem < t ) ) {
			sglib___StorageItem_add_recursive( &t->left[index], elem, index );
			if( ( t->color_field[index] ) == 0 )
				sglib___StorageItem_fix_left_insertion_discrepancy( tree, index );
		}
		else {
			sglib___StorageItem_add_recursive( &t->right[index], elem, index );
			if( ( t->color_field[index] ) == 0 )
				sglib___StorageItem_fix_right_insertion_discrepancy( tree, index );
		}
	}
}
static int
sglib___StorageItem_delete_rightmost_leaf( StorageItem** tree, StorageItem** theLeaf, int index )
{
	StorageItem* t;
	int res, deepDecreased;
	t = *tree;
	res = 0;
	( (void)sizeof( ( t != NULL ) ? 1 : 0 ), __extension__( {
		  if( t != NULL )
			  ;
		  else
			  __assert_fail(
				  "t!=NULL", "src/server/storage.c", 16, __extension__ __PRETTY_FUNCTION__ );
	  } ) );
	if( t->right[index] == NULL ) {
		*theLeaf = t;
		if( t->left[index] != NULL ) {
			if( ( t->color_field[index] ) == 0 && ( t->left[index]->color_field[index] ) == 0 )
				res = 1;
			{
				( t->left[index]->color_field[index] ) = ( 0 );
			};
			*tree = t->left[index];
		}
		else {
			*tree = NULL;
			res = ( ( t->color_field[index] ) == 0 );
		}
	}
	else {
		deepDecreased =
			sglib___StorageItem_delete_rightmost_leaf( &t->right[index], theLeaf, index );
		if( deepDecreased )
			res = sglib___StorageItem_fix_right_deletion_discrepancy( tree, index );
	}
	return ( res );
}

int sglib___StorageItem_delete_recursive( StorageItem** tree, StorageItem* elem, int index )
{
	StorageItem *t, *theLeaf;
	int cmp, res, deepDecreased;
	t = *tree;
	res = 0;
	if( t == NULL ) {
		( (void)sizeof(
			  ( 0 &&
				"The element to delete not found in the tree,  use 'delete_if_member'" != NULL )
				  ? 1
				  : 0 ),
		  __extension__( {
			  if( 0 &&
				  "The element to delete not found in the tree,  use 'delete_if_member'" != NULL )
				  ;
			  else
				  __assert_fail( "0 && \"The element to delete not found in the tree,  use "
								 "'delete_if_member'\"!=NULL",
								 "src/server/storage.c",
								 16,
								 __extension__ __PRETTY_FUNCTION__ );
		  } ) );
	}
	else {
		cmp = storage_item_cmp( elem, t, index );
		if( cmp < 0 || ( cmp == 0 && elem < t ) ) {
			deepDecreased = sglib___StorageItem_delete_recursive( &t->left[index], elem, index );
			if( deepDecreased ) {
				res = sglib___StorageItem_fix_left_deletion_discrepancy( tree, index );
			}
		}
		else if( cmp > 0 || ( cmp == 0 && elem > t ) ) {
			deepDecreased = sglib___StorageItem_delete_recursive( &t->right[index], elem, index );
			if( deepDecreased ) {
				res = sglib___StorageItem_fix_right_deletion_discrepancy( tree, index );
			}
		}
		else {
			( (void)sizeof( ( elem == t && "Deleting an element which is non member of the tree, "
										   "use 'delete_if_member'" != NULL )
								? 1
								: 0 ),
			  __extension__( {
				  if( elem == t && "Deleting an element which is non member of the tree, use "
								   "'delete_if_member'" != NULL )
					  ;
				  else
					  __assert_fail(
						  "elem==t && \"Deleting an element which is non member of the tree, use "
						  "'delete_if_member'\"!=NULL",
						  "src/server/storage.c",
						  16,
						  __extension__ __PRETTY_FUNCTION__ );
			  } ) );
			if( t->left[index] == NULL ) {
				if( t->right[index] == NULL ) {
					*tree = NULL;
					res = ( ( t->color_field[index] ) == 0 );
				}
				else {
					if( ( t->color_field[index] ) == 0 &&
						( t->right[index]->color_field[index] ) == 0 )
						res = 1;
					{
						( t->right[index]->color_field[index] ) = ( 0 );
					};
					*tree = t->right[index];
				}
			}
			else {
				deepDecreased =
					sglib___StorageItem_delete_rightmost_leaf( &t->left[index], &theLeaf, index );
				theLeaf->left[index] = t->left[index];
				theLeaf->right[index] = t->right[index];
				{
					( theLeaf->color_field[index] ) = ( ( t->color_field[index] ) );
				};
				*tree = theLeaf;
				if( deepDecreased )
					res = sglib___StorageItem_fix_left_deletion_discrepancy( tree, index );
			}
		}
	}
	return ( res );
}

void sglib_StorageItem_add( StorageItem** tree, StorageItem* elem, int index )
{
	elem->left[index] = elem->right[index] = NULL;
	sglib___StorageItem_add_recursive( tree, elem, index );
	{
		( ( *tree )->color_field[index] ) = ( 0 );
	};
}

void sglib_StorageItem_delete( StorageItem** tree, StorageItem* elem, int index )
{
	sglib___StorageItem_delete_recursive( tree, elem, index );
	if( *tree != NULL ) {
		( ( *tree )->color_field[index] ) = ( 0 );
	};
}

StorageItem* sglib_StorageItem_find_member( StorageItem* t, StorageItem* elem, int index )
{
	StorageItem* res;
	{
		StorageItem* _s_;
		int _c_;
		_s_ = ( t );
		while( _s_ != NULL ) {
			_c_ = storage_item_cmp( ( elem ), _s_, index );
			if( _c_ < 0 )
				_s_ = _s_->left[index];
			else if( _c_ > 0 )
				_s_ = _s_->right[index];
			else
				break;
		}
		( res ) = _s_;
	};
	return ( res );
}

StorageItem*
sglib_StorageItem_find_member_or_nearest_lesser( StorageItem* t, StorageItem* elem, int index )
{
	StorageItem *s, *nearest = NULL;
	int c;
	s = ( t );
	while( s != NULL ) {
		c = storage_item_cmp( elem, s, index );
		if( c < 0 ) {
			s = s->left[index];
		}
		else if( c > 0 ) {
			nearest = s;
			s = s->right[index];
		}
		else {
			nearest = s;
			break;
		}
	}
	return nearest;
}

int sglib_StorageItem_is_member( StorageItem* t, StorageItem* elem, int index )
{
	int cmp;
	while( t != NULL ) {
		cmp = storage_item_cmp( elem, t, index );
		if( cmp < 0 || ( cmp == 0 && elem < t ) ) {
			t = t->left[index];
		}
		else if( cmp > 0 || ( cmp == 0 && elem > t ) ) {
			t = t->right[index];
		}
		else {
			( (void)sizeof( ( t == elem ) ? 1 : 0 ), __extension__( {
				  if( t == elem )
					  ;
				  else
					  __assert_fail( "t == elem",
									 "src/server/storage.c",
									 16,
									 __extension__ __PRETTY_FUNCTION__ );
			  } ) );
			return ( 1 );
		}
	}
	return ( 0 );
}

int sglib_StorageItem_delete_if_member( StorageItem** tree,
										StorageItem* elem,
										StorageItem** memb,
										int index )
{
	if( ( *memb = sglib_StorageItem_find_member( *tree, elem, index ) ) != NULL ) {
		sglib_StorageItem_delete( tree, *memb, index );
		return ( 1 );
	}
	else {
		return ( 0 );
	}
}

int sglib_StorageItem_add_if_not_member( StorageItem** tree,
										 StorageItem* elem,
										 StorageItem** memb,
										 int index )
{
	if( ( *memb = sglib_StorageItem_find_member( *tree, elem, index ) ) == NULL ) {
		sglib_StorageItem_add( tree, elem, index );
		return ( 1 );
	}
	else {
		return ( 0 );
	}
}

int sglib_StorageItem_len( StorageItem* t, int index )
{
	int n;
	StorageItem* e;
	(void)( e );
	n = 0;
	{
		{
			StorageItem* _path_[128];
			StorageItem* _right_[128];
			char _pass_[128];
			StorageItem* _cn_;
			int _pathi_;
			StorageItem* e;
			(void)( e );
			_cn_ = ( t );
			_pathi_ = 0;
			while( _cn_ != NULL ) {
				while( _cn_ != NULL ) {
					_path_[_pathi_] = _cn_;
					_right_[_pathi_] = _cn_->right[index];
					_pass_[_pathi_] = 0;
					_cn_ = _cn_->left[index];
					if( 1 == 0 ) {
						e = _path_[_pathi_];
						{
							n++;
						}
					}
					_pathi_++;
					if( _pathi_ >= 128 )
						( (void)sizeof( ( 0 && "the binary_tree is too deep" ) ? 1 : 0 ),
						  __extension__( {
							  if( 0 && "the binary_tree is too deep" )
								  ;
							  else
								  __assert_fail( "0 && \"the binary_tree is too deep\"",
												 "src/server/storage.c",
												 16,
												 __extension__ __PRETTY_FUNCTION__ );
						  } ) );
				}
				do {
					_pathi_--;
					if( ( 1 == 1 && _pass_[_pathi_] == 0 ) ||
						( 1 == 2 && ( _pass_[_pathi_] == 1 || _right_[_pathi_] == NULL ) ) ) {
						e = _path_[_pathi_];
						{
							n++;
						}
					}
					_pass_[_pathi_]++;
				} while( _pathi_ > 0 && _right_[_pathi_] == NULL );
				_cn_ = _right_[_pathi_];
				_right_[_pathi_] = NULL;
				_pathi_++;
			}
		};
	};
	return ( n );
}

void sglib__StorageItem_it_compute_current_elem( struct sglib_StorageItem_iterator* it, int index )
{
	int i, j;
	StorageItem *s, *eqt;
	int ( *subcomparator )( StorageItem*, StorageItem* );
	eqt = it->equalto;
	subcomparator = it->subcomparator;
	it->currentelem = NULL;
	while( it->pathi > 0 && it->currentelem == NULL ) {
		i = it->pathi - 1;
		if( i >= 0 ) {
			if( it->pass[i] >= 2 ) {
				it->pathi--;
			}
			else {
				if( it->pass[i] == 0 ) {
					s = it->path[i]->left[index];
				}
				else {
					s = it->path[i]->right[index];
				}
				if( eqt != NULL ) {
					if( subcomparator == NULL ) {
						{
							StorageItem* _s_;
							int _c_;
							_s_ = ( s );
							while( _s_ != NULL ) {
								_c_ = storage_item_cmp( ( eqt ), _s_, index );
								if( _c_ < 0 )
									_s_ = _s_->left[index];
								else if( _c_ > 0 )
									_s_ = _s_->right[index];
								else
									break;
							}
							( s ) = _s_;
						};
					}
					else {
						{
							StorageItem* _s_;
							int _c_;
							_s_ = ( s );
							while( _s_ != NULL ) {
								_c_ = subcomparator( ( eqt ), _s_ );
								if( _c_ < 0 )
									_s_ = _s_->left[index];
								else if( _c_ > 0 )
									_s_ = _s_->right[index];
								else
									break;
							}
							( s ) = _s_;
						};
					}
				}
				if( s != NULL ) {
					j = i + 1;
					it->path[j] = s;
					it->pass[j] = 0;
					it->pathi++;
				}
				it->pass[i]++;
			}
		}
		if( it->pathi > 0 && it->order == it->pass[it->pathi - 1] ) {
			it->currentelem = it->path[it->pathi - 1];
		}
	}
}

StorageItem* sglib__StorageItem_it_init( struct sglib_StorageItem_iterator* it,
										 StorageItem* tree,
										 int order,
										 int ( *subcomparator )( StorageItem*, StorageItem* ),
										 StorageItem* equalto,
										 int index )
{
	StorageItem* t;
	( (void)sizeof( ( it != NULL ) ? 1 : 0 ), __extension__( {
		  if( it != NULL )
			  ;
		  else
			  __assert_fail(
				  "it!=NULL", "src/server/storage.c", 16, __extension__ __PRETTY_FUNCTION__ );
	  } ) );
	it->order = order;
	it->equalto = equalto;
	it->subcomparator = subcomparator;
	if( equalto == NULL ) {
		t = tree;
	}
	else {
		if( subcomparator == NULL ) {
			{
				StorageItem* _s_;
				int _c_;
				_s_ = ( tree );
				while( _s_ != NULL ) {
					_c_ = storage_item_cmp( ( equalto ), _s_, index );
					if( _c_ < 0 )
						_s_ = _s_->left[index];
					else if( _c_ > 0 )
						_s_ = _s_->right[index];
					else
						break;
				}
				( t ) = _s_;
			};
		}
		else {
			{
				StorageItem* _s_;
				int _c_;
				_s_ = ( tree );
				while( _s_ != NULL ) {
					_c_ = subcomparator( ( equalto ), _s_ );
					if( _c_ < 0 ) {
						_s_ = _s_->left[index];
					}
					else if( _c_ > 0 ) {
						_s_ = _s_->right[index];
					}
					else {
						break;
					}
				}
				( t ) = _s_;
			};
		}
	}
	if( t == NULL ) {
		it->pathi = 0;
		it->currentelem = NULL;
	}
	else {
		it->pathi = 1;
		it->pass[0] = 0;
		it->path[0] = t;
		if( order == 0 ) {
			it->currentelem = t;
		}
		else {
			sglib__StorageItem_it_compute_current_elem( it, index );
		}
	}
	return ( it->currentelem );
}

StorageItem*
sglib_StorageItem_it_init( struct sglib_StorageItem_iterator* it, StorageItem* tree, int index )
{
	return ( sglib__StorageItem_it_init( it, tree, 2, NULL, NULL, index ) );
}

StorageItem* sglib_StorageItem_it_init_preorder( struct sglib_StorageItem_iterator* it,
												 StorageItem* tree,
												 int index )
{
	return ( sglib__StorageItem_it_init( it, tree, 0, NULL, NULL, index ) );
}

StorageItem* sglib_StorageItem_it_init_inorder( struct sglib_StorageItem_iterator* it,
												StorageItem* tree,
												int index )
{
	return ( sglib__StorageItem_it_init( it, tree, 1, NULL, NULL, index ) );
}

StorageItem* sglib_StorageItem_it_init_postorder( struct sglib_StorageItem_iterator* it,
												  StorageItem* tree,
												  int index )
{
	return ( sglib__StorageItem_it_init( it, tree, 2, NULL, NULL, index ) );
}

StorageItem* sglib_StorageItem_it_init_on_equal( struct sglib_StorageItem_iterator* it,
												 StorageItem* tree,
												 int ( *subcomparator )( StorageItem*,
																		 StorageItem* ),
												 StorageItem* equalto,
												 int index )
{
	return ( sglib__StorageItem_it_init( it, tree, 1, subcomparator, equalto, index ) );
}

StorageItem* sglib_StorageItem_it_current( struct sglib_StorageItem_iterator* it, int index )
{
	return ( it->currentelem );
}

StorageItem* sglib_StorageItem_it_next( struct sglib_StorageItem_iterator* it, int index )
{
	sglib__StorageItem_it_compute_current_elem( it, index );
	return ( it->currentelem );
}

static void sglib___StorageItem_consistency_check_recursive( StorageItem* t,
															 int* pathdeep,
															 int cdeep,
															 int index )
{
	if( t == NULL ) {
		if( *pathdeep < 0 )
			*pathdeep = cdeep;
		else
			( (void)sizeof( ( *pathdeep == cdeep ) ? 1 : 0 ), __extension__( {
				  if( *pathdeep == cdeep )
					  ;
				  else
					  __assert_fail( "*pathdeep == cdeep",
									 "src/server/storage.c",
									 16,
									 __extension__ __PRETTY_FUNCTION__ );
			  } ) );
	}
	else {
		if( t->left[index] != NULL )
			( (void)sizeof( ( storage_item_cmp( t->left[index], t, index ) <= 0 ) ? 1 : 0 ),
			  __extension__( {
				  if( storage_item_cmp( t->left[index], t, index ) <= 0 )
					  ;
				  else
					  __assert_fail( "storage_item_cmp(t->left[index], t) <= 0",
									 "src/server/storage.c",
									 16,
									 __extension__ __PRETTY_FUNCTION__ );
			  } ) );
		if( t->right[index] != NULL )
			( (void)sizeof( ( storage_item_cmp( t, t->right[index], index ) <= 0 ) ? 1 : 0 ),
			  __extension__( {
				  if( storage_item_cmp( t, t->right[index], index ) <= 0 )
					  ;
				  else
					  __assert_fail( "storage_item_cmp(t, t->right[index]) <= 0",
									 "src/server/storage.c",
									 16,
									 __extension__ __PRETTY_FUNCTION__ );
			  } ) );
		if( ( t->color_field[index] ) == 1 ) {
			( (void)sizeof(
				  ( t->left[index] == NULL || ( t->left[index]->color_field[index] ) == 0 ) ? 1
																							: 0 ),
			  __extension__( {
				  if( t->left[index] == NULL || ( t->left[index]->color_field[index] ) == 0 )
					  ;
				  else
					  __assert_fail( "t->left[index] == NULL || "
									 "SGLIB___GET_VALUE(t->left[index]->color_field)==0",
									 "src/server/storage.c",
									 16,
									 __extension__ __PRETTY_FUNCTION__ );
			  } ) );
			( (void)sizeof(
				  ( t->right[index] == NULL || ( t->right[index]->color_field[index] ) == 0 ) ? 1
																							  : 0 ),
			  __extension__( {
				  if( t->right[index] == NULL || ( t->right[index]->color_field[index] ) == 0 )
					  ;
				  else
					  __assert_fail( "t->right[index] == NULL || "
									 "SGLIB___GET_VALUE(t->right[index]->color_field)==0",
									 "src/server/storage.c",
									 16,
									 __extension__ __PRETTY_FUNCTION__ );
			  } ) );
			sglib___StorageItem_consistency_check_recursive(
				t->left[index], pathdeep, cdeep, index );
			sglib___StorageItem_consistency_check_recursive(
				t->right[index], pathdeep, cdeep, index );
		}
		else {
			sglib___StorageItem_consistency_check_recursive(
				t->left[index], pathdeep, cdeep + 1, index );
			sglib___StorageItem_consistency_check_recursive(
				t->right[index], pathdeep, cdeep + 1, index );
		}
	}
}
void sglib___StorageItem_consistency_check( StorageItem* t, int index )
{
	int pathDeep;
	( (void)sizeof( ( t == NULL || ( t->color_field[index] ) == 0 ) ? 1 : 0 ), __extension__( {
		  if( t == NULL || ( t->color_field[index] ) == 0 )
			  ;
		  else
			  __assert_fail( "t==NULL || SGLIB___GET_VALUE(t->color_field) == 0",
							 "src/server/storage.c",
							 16,
							 __extension__ __PRETTY_FUNCTION__ );
	  } ) );
	pathDeep = -1;
	sglib___StorageItem_consistency_check_recursive( t, &pathDeep, 0, index );
}
